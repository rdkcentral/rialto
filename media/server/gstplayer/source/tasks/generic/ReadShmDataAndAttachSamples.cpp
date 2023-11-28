/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tasks/generic/ReadShmDataAndAttachSamples.h"
#include "GenericPlayerContext.h"
#include "IDataReader.h"
#include "IGstGenericPlayerPrivate.h"
#include "IMediaPipeline.h"
#include "RialtoServerLogging.h"

#include <gst_svp_meta.h>
#include <gst/app/gstappsrc.h>
#include <gst/audio/streamvolume.h>
#include <gst/base/gstbasetransform.h>
#include <gst/base/gstbytewriter.h>
#include <gst/gst.h>

namespace firebolt::rialto::server::tasks::generic
{
ReadShmDataAndAttachSamples::ReadShmDataAndAttachSamples(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                         const std::shared_ptr<IDataReader> &dataReader)
    : m_context{context}, m_player{player}, m_dataReader{dataReader}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing ReadShmDataAndAttachSamples");
}

ReadShmDataAndAttachSamples::~ReadShmDataAndAttachSamples()
{
    RIALTO_SERVER_LOG_DEBUG("ReadShmDataAndAttachSamples finished");
}

void ReadShmDataAndAttachSamples::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing ReadShmDataAndAttachSamples");
    // Read media segments from shared memory
    IMediaPipeline::MediaSegmentVector mediaSegments = m_dataReader->readData();

    static void *svpCtx = nullptr;

    if (!svpCtx)
        gst_svp_ext_get_context(&svpCtx, Server, 0);

    for (const auto &mediaSegment : mediaSegments)
    {
        GstBuffer *gstBuffer = m_player.createBuffer(*mediaSegment);
        if (mediaSegment->getType() == firebolt::rialto::MediaSourceType::VIDEO)
        {
            try
            {
                IMediaPipeline::MediaSegmentVideo &videoSegment =
                    dynamic_cast<IMediaPipeline::MediaSegmentVideo &>(*mediaSegment);
                m_player.updateVideoCaps(videoSegment.getWidth(), videoSegment.getHeight(), videoSegment.getFrameRate(),
                                         videoSegment.getCodecData());

                if (!videoSegment.getSecureToken().empty())
                {
                    if (videoSegment.getSecureToken().size() == svp_token_size())
                    {
                        //todo: check if svp context not null
                        auto subsamplesRawSize = videoSegment.getSubSamples().size() *
                                                 (sizeof(guint16) + sizeof(guint32));
                        guint8 *subsamplesRaw = static_cast<guint8 *>(g_malloc(subsamplesRawSize));
                        GstByteWriter writer;
                        gst_byte_writer_init_with_data(&writer, subsamplesRaw, subsamplesRawSize, FALSE);

                        for (const auto &subSample : videoSegment.getSubSamples())
                        {
                            gst_byte_writer_put_uint16_be(&writer, subSample.numClearBytes);
                            gst_byte_writer_put_uint32_be(&writer, subSample.numEncryptedBytes);
                        }
                        GstBuffer *subsamples = gst_buffer_new_wrapped(subsamplesRaw, subsamplesRawSize);

                        void *secToken = nullptr;
                        svp_buffer_alloc_token(&secToken);
                        memcpy(secToken, videoSegment.getSecureToken().data(), svp_token_size());

                        gst_buffer_append_svp_transform(svpCtx, gstBuffer, subsamples,
                                                        videoSegment.getSubSamples().size(),
                                                        static_cast<uint8_t *>(secToken), mediaSegment->getDataLength());
                        //todo: make sure how to process clear buffers on amlogic
                        //gst_buffer_svp_transform_from_cleardata(svpCtx, gstBuffer, Video);
                        if (subsamples)
                        {
                            gst_buffer_unref(subsamples);
                        }

                        svp_buffer_free_token(secToken);
                        RIALTO_SERVER_LOG_DEBUG("Added secure token to the buffer");
                    }
                    else
                    {
                        RIALTO_SERVER_LOG_ERROR("Secure token has invalid size %u", videoSegment.getSecureToken().size());
                    }
                }
            }
            catch (const std::exception &e)
            {
                RIALTO_SERVER_LOG_ERROR("Failed to get the video segment, reason: %s", e.what());
            }

            m_context.videoBuffers.push_back(gstBuffer);
            m_player.attachVideoData();
        }
        else if (mediaSegment->getType() == firebolt::rialto::MediaSourceType::AUDIO)
        {
            try
            {
                IMediaPipeline::MediaSegmentAudio &audioSegment =
                    dynamic_cast<IMediaPipeline::MediaSegmentAudio &>(*mediaSegment);
                m_player.updateAudioCaps(audioSegment.getSampleRate(), audioSegment.getNumberOfChannels(),
                                         audioSegment.getCodecData());
            }
            catch (const std::exception &e)
            {
                RIALTO_SERVER_LOG_ERROR("Failed to get the audio segment, reason: %s", e.what());
            }

            m_context.audioBuffers.push_back(gstBuffer);
            m_player.attachAudioData();
        }
    }
    // All segments in vector have the same type
    m_player.notifyNeedMediaData((!mediaSegments.empty() &&
                                  mediaSegments.front()->getType() == firebolt::rialto::MediaSourceType::AUDIO),
                                 (!mediaSegments.empty() &&
                                  mediaSegments.front()->getType() == firebolt::rialto::MediaSourceType::VIDEO));
}
} // namespace firebolt::rialto::server::tasks::generic
