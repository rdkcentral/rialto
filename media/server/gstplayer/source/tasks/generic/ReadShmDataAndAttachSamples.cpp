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
