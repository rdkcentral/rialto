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

#include "tasks/AttachSource.h"
#include "IGlibWrapper.h"
#include "IGstWrapper.h"
#include "IMediaPipeline.h"
#include "RialtoServerLogging.h"

namespace
{
std::string getSampleRateAndChannelsAsCapsStr(const firebolt::rialto::IMediaPipeline::MediaSource &source)
{
    std::string capsStr;
    firebolt::rialto::AudioConfig audioConfig;
    if (source.getAudioConfig(audioConfig))
    {
        if (audioConfig.numberOfChannels != firebolt::rialto::kInvalidAudioChannels)
            capsStr += ", channels=" + std::to_string(audioConfig.numberOfChannels);

        if (audioConfig.sampleRate != firebolt::rialto::kInvalidAudioSampleRate)
            capsStr += ", rate=" + std::to_string(audioConfig.sampleRate);
    }

    return capsStr;
}

std::string getAudioSpecificConfiguratuionAsCapsStr(std::shared_ptr<firebolt::rialto::server::IGstWrapper> gstWrapper,
                                                    std::shared_ptr<firebolt::rialto::server::IGlibWrapper> glibWrapper,
                                                    const firebolt::rialto::IMediaPipeline::MediaSource &source)
{
    std::string capsStr;
    firebolt::rialto::AudioConfig audioConfig;
    if (source.getAudioConfig(audioConfig) && audioConfig.codecSpecificConfig.size())
    {
        gsize codec_priv_size = audioConfig.codecSpecificConfig.size();
        gconstpointer codec_priv = audioConfig.codecSpecificConfig.data();

        GstCaps *gst_caps = gstWrapper->gstCodecUtilsOpusCreateCapsFromHeader(codec_priv, codec_priv_size);
        gchar *caps_str = gstWrapper->gstCapsToString(gst_caps);
        capsStr = caps_str;
        glibWrapper->gFree(caps_str);
        gstWrapper->gstCapsUnref(gst_caps);
    }

    return capsStr;
}

std::string createCapsStr(std::shared_ptr<firebolt::rialto::server::IGstWrapper> gstWrapper,
                          std::shared_ptr<firebolt::rialto::server::IGlibWrapper> glibWrapper,
                          const firebolt::rialto::IMediaPipeline::MediaSource &source)
{
    auto mimeType = source.getMimeType();
    if (mimeType.compare("video/x-h265") == 0)
    {
        return "video/x-h265";
    }
    else if (mimeType.compare("video/x-h264") == 0)
    {
        switch (source.getSegmentAlignment())
        {
        case firebolt::rialto::SegmentAlignment::AU:
            return "video/x-h264, stream-format=byte-stream, alignment=au";
        case firebolt::rialto::SegmentAlignment::NAL:
            return "video/x-h264, stream-format=byte-stream, alignment=nal";
        default:
            return "video/x-h265";
        }
    }
    else if (mimeType.compare("video/mpeg2") == 0)
    {
        return "video/mpeg, mpegversion=(int) 2";
    }
    else if (mimeType.compare("ideo/x-theora") == 0)
    {
        return "ideo/x-theora";
    }
    else if (mimeType.compare("video/x-vc1") == 0)
    {
        return "video/x-vc1";
    }
    else if (mimeType.compare("video/x-av1") == 0)
    {
        return "video/x-av1";
    }
    else if (mimeType.compare("video/x-vp8") == 0)
    {
        return "video/x-vp8";
    }
    else if (mimeType.compare("video/x-vp9") == 0)
    {
        return "video/x-vp9";
    }
    else if (mimeType.compare("audio/mpeg") == 0)
    {
        std::string capsStr = "audio/mpeg, mpegversion=4";
        capsStr += getSampleRateAndChannelsAsCapsStr(source);
        return capsStr;
    }
    else if (mimeType.compare("audio/x-eac3") == 0)
    {
        std::string capsStr = "audio/x-eac3";
        capsStr += getSampleRateAndChannelsAsCapsStr(source);
        return capsStr;
    }
    else if (mimeType.compare("audio/x-vorbis") == 0)
    {
        return "audio/x-vorbis";
    }
    else if (mimeType.compare("audio/x-opus") == 0)
    {
        std::string capsStr = getAudioSpecificConfiguratuionAsCapsStr(gstWrapper, glibWrapper, source);
        if (capsStr.empty())
            return "audio/x-opus, channel-mapping-family=0";
        else
            return capsStr;
    }
    return "";
}

} // namespace

namespace firebolt::rialto::server
{
AttachSource::AttachSource(PlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper,
                           std::shared_ptr<IGlibWrapper> glibWrapper, const IMediaPipeline::MediaSource &source)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_attachedSource{source}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing AttachSource");
}

AttachSource::~AttachSource()
{
    RIALTO_SERVER_LOG_DEBUG("AttachSource finished");
}

void AttachSource::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing AttachSource");
    auto strCaps = createCapsStr(m_gstWrapper, m_glibWrapper, m_attachedSource);
    RIALTO_SERVER_LOG_DEBUG("caps str: %s ", strCaps.c_str());
    GstCaps *caps = m_gstWrapper->gstCapsFromString(strCaps.c_str());

    auto elem = m_context.streamInfo.find(m_attachedSource.getType());
    if (elem == m_context.streamInfo.end())
    {
        GstElement *appSrc = nullptr;
        if (m_attachedSource.getType() == MediaSourceType::AUDIO)
        {
            RIALTO_SERVER_LOG_MIL("Adding Audio appsrc");
            appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "audsrc");
        }
        else if (m_attachedSource.getType() == MediaSourceType::VIDEO)
        {
            RIALTO_SERVER_LOG_MIL("Adding Video appsrc");
            appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "vidsrc");
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Unknown media source type");
            if (caps)
                m_gstWrapper->gstCapsUnref(caps);
            return;
        }

        m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(appSrc), caps);
        m_context.streamInfo.emplace(m_attachedSource.getType(), appSrc);
    }
    else
    {
        GstCaps *appsrcCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(elem->second));
        if ((!appsrcCaps) || (!m_gstWrapper->gstCapsIsEqual(appsrcCaps, caps)))
        {
            gchar *capsStr = m_gstWrapper->gstCapsToString(caps);
            RIALTO_SERVER_LOG_MIL("Updating %s appsrc caps to '%s'",
                                  m_attachedSource.getType() == MediaSourceType::AUDIO ? "Audio" : "Video", capsStr);
            m_glibWrapper->gFree(capsStr);
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(elem->second), caps);
        }

        if (appsrcCaps)
            m_gstWrapper->gstCapsUnref(appsrcCaps);
    }

    if (caps)
        m_gstWrapper->gstCapsUnref(caps);
}
} // namespace firebolt::rialto::server
