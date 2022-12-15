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
#include <unordered_map>

namespace firebolt::rialto::server
{
AttachSource::AttachSource(PlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper,
                           std::shared_ptr<IGlibWrapper> glibWrapper, std::unique_ptr<IMediaPipeline::MediaSource> &source)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_attachedSource{source->copy()}
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

    if (m_attachedSource->getType() == MediaSourceType::UNKNOWN)
    {
        RIALTO_SERVER_LOG_ERROR("Unknown media source type");
        return;
    }

    GstCaps *caps = createCapsFromMediaSource();
    gchar *capsStr = m_gstWrapper->gstCapsToString(caps);
    std::string strCaps = capsStr;
    m_glibWrapper->gFree(capsStr);

    RIALTO_SERVER_LOG_DEBUG("caps str: '%s'", strCaps.c_str());

    auto elem = m_context.streamInfo.find(m_attachedSource->getType());
    if (elem == m_context.streamInfo.end())
    {
        GstElement *appSrc = nullptr;
        if (m_attachedSource->getType() == MediaSourceType::AUDIO)
        {
            RIALTO_SERVER_LOG_MIL("Adding Audio appsrc");
            appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "audsrc");
        }
        else if (m_attachedSource->getType() == MediaSourceType::VIDEO)
        {
            RIALTO_SERVER_LOG_MIL("Adding Video appsrc");
            appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "vidsrc");
        }

        m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(appSrc), caps);
        m_context.streamInfo.emplace(m_attachedSource->getType(), appSrc);
    }
    else
    {
        GstCaps *appsrcCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(elem->second));
        if ((!appsrcCaps) || (!m_gstWrapper->gstCapsIsEqual(appsrcCaps, caps)))
        {
            RIALTO_SERVER_LOG_MIL("Updating %s appsrc caps to '%s'",
                                  m_attachedSource->getType() == MediaSourceType::AUDIO ? "Audio" : "Video",
                                  strCaps.c_str());
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(elem->second), caps);
        }

        if (appsrcCaps)
            m_gstWrapper->gstCapsUnref(appsrcCaps);
    }

    if (caps)
        m_gstWrapper->gstCapsUnref(caps);
}

GstCaps *AttachSource::createSimpleCapsFromMimeType(const std::string &mimeType) const
{
    static const std::unordered_map<std::string, std::string> mimeToMediaType =
        {{"video/h264", "video/x-h264"},   {"video/h265", "video/x-h265"},  {"video/x-av1", "video/x-av1"},
         {"video/x-vp9", "video/x-vp9"},   {"audio/mp4", "audio/mpeg"},     {"audio/aac", "audio/mpeg"},
         {"audio/x-eac3", "audio/x-eac3"}, {"audio/x-opus", "audio/x-opus"}};

    auto mimeToMediaTypeIt = mimeToMediaType.find(mimeType);
    if (mimeToMediaTypeIt != mimeToMediaType.end())
    {
        return m_gstWrapper->gstCapsNewEmptySimple(mimeToMediaTypeIt->second.c_str());
    }

    return m_gstWrapper->gstCapsNewEmpty();
}

GstCaps *AttachSource::getAudioSpecificConfiguration() const
{
    GstCaps *caps = nullptr;
    firebolt::rialto::AudioConfig audioConfig;
    if (m_attachedSource->getAudioConfig(audioConfig) && audioConfig.codecSpecificConfig.size())
    {
        gsize codec_priv_size = audioConfig.codecSpecificConfig.size();
        gconstpointer codec_priv = audioConfig.codecSpecificConfig.data();

        caps = m_gstWrapper->gstCodecUtilsOpusCreateCapsFromHeader(codec_priv, codec_priv_size);
        if (!caps)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to parse opus header");
        }
    }

    return caps;
}

void AttachSource::addAlignmentToCaps(GstCaps *caps) const
{
    static const std::unordered_map<firebolt::rialto::SegmentAlignment, std::string> aligmentMap =
        {{firebolt::rialto::SegmentAlignment::AU, "au"}, {firebolt::rialto::SegmentAlignment::NAL, "nal"}};

    auto aligmentMapIt = aligmentMap.find(m_attachedSource->getSegmentAlignment());
    if (aligmentMapIt != aligmentMap.end())
    {
        m_gstWrapper->gstCapsSetSimple(caps, "alignment", G_TYPE_STRING, aligmentMapIt->second.c_str(), nullptr);
    }
}

void AttachSource::addCodecDataToCaps(GstCaps *caps) const
{
    const std::vector<uint8_t> &codecData = m_attachedSource->getCodecData();
    if (!codecData.empty())
    {
        gpointer memory = m_glibWrapper->gMemdup(codecData.data(), codecData.size());
        GstBuffer *buf = m_gstWrapper->gstBufferNewWrapped(memory, codecData.size());
        m_gstWrapper->gstCapsSetSimple(caps, "codec_data", GST_TYPE_BUFFER, buf, nullptr);
        m_gstWrapper->gstBufferUnref(buf);
    }
}

void AttachSource::addStreamFormatToCaps(GstCaps *caps) const
{
    static const std::unordered_map<firebolt::rialto::StreamFormat, std::string> formatMap =
        {{firebolt::rialto::StreamFormat::RAW, "raw"},
         {firebolt::rialto::StreamFormat::AVC, "avc"},
         {firebolt::rialto::StreamFormat::BYTE_STREAM, "byte-stream"}};

    auto formatMapIt = formatMap.find(m_attachedSource->getStreamFormat());
    if (formatMapIt != formatMap.end())
    {
        m_gstWrapper->gstCapsSetSimple(caps, "stream-format", G_TYPE_STRING, formatMapIt->second.c_str(), nullptr);
    }
}

void AttachSource::addSampleRateAndChannelsToCaps(GstCaps *caps) const
{
    firebolt::rialto::AudioConfig audioConfig;
    if (m_attachedSource->getAudioConfig(audioConfig))
    {
        if (audioConfig.numberOfChannels != firebolt::rialto::kInvalidAudioChannels)
            m_gstWrapper->gstCapsSetSimple(caps, "channels", G_TYPE_INT, audioConfig.numberOfChannels, NULL);

        if (audioConfig.sampleRate != firebolt::rialto::kInvalidAudioSampleRate)
            m_gstWrapper->gstCapsSetSimple(caps, "rate", G_TYPE_INT, audioConfig.sampleRate, NULL);
    }
}

void AttachSource::addMpegVersionToCaps(GstCaps *caps) const
{
    m_gstWrapper->gstCapsSetSimple(caps, "mpegversion", G_TYPE_INT, 4, NULL);
}

GstCaps *AttachSource::createCapsFromMediaSource() const
{
    std::string mimeType = m_attachedSource->getMimeType();
    if (mimeType == "audio/x-opus")
    {
        GstCaps *caps = getAudioSpecificConfiguration();
        if (caps)
            return caps;

        return m_gstWrapper->gstCapsNewSimple("audio/x-opus", "channel-mapping-family", G_TYPE_INT, 0, nullptr);
    }

    GstCaps *caps = createSimpleCapsFromMimeType(mimeType);

    if (mimeType == "audio/mp4" || mimeType == "audio/aac")
    {
        addMpegVersionToCaps(caps);
    }

    addAlignmentToCaps(caps);
    addCodecDataToCaps(caps);
    addStreamFormatToCaps(caps);

    if (mimeType.find("audio/") == 0)
    {
        addSampleRateAndChannelsToCaps(caps);
    }

    return caps;
}
} // namespace firebolt::rialto::server
