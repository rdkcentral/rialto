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
namespace
{
class MediaSourceCapsBuilder
{
public:
    MediaSourceCapsBuilder(std::shared_ptr<IGstWrapper> gstWrapper, std::shared_ptr<IGlibWrapper> glibWrapper,
                           const IMediaPipeline::MediaSource &source)
        : m_gstWrapper(gstWrapper), m_glibWrapper(glibWrapper), m_attachedSource(source)
    {
    }
    virtual GstCaps *buildCaps() { return buildCommonCaps(); }

protected:
    GstCaps *buildCommonCaps()
    {
        GstCaps *caps = createSimpleCapsFromMimeType();

        addAlignmentToCaps(caps);
        addCodecDataToCaps(caps);
        addStreamFormatToCaps(caps);

        return caps;
    }

    GstCaps *createSimpleCapsFromMimeType() const
    {
        static const std::unordered_map<std::string, std::string> mimeToMediaType =
            {{"video/h264", "video/x-h264"},   {"video/h265", "video/x-h265"},  {"video/x-av1", "video/x-av1"},
             {"video/x-vp9", "video/x-vp9"},   {"audio/mp4", "audio/mpeg"},     {"audio/aac", "audio/mpeg"},
             {"audio/x-eac3", "audio/x-eac3"}, {"audio/x-opus", "audio/x-opus"}};

        auto mimeToMediaTypeIt = mimeToMediaType.find(m_attachedSource.getMimeType());
        if (mimeToMediaTypeIt != mimeToMediaType.end())
        {
            return m_gstWrapper->gstCapsNewEmptySimple(mimeToMediaTypeIt->second.c_str());
        }

        return m_gstWrapper->gstCapsNewEmpty();
    }

    void addAlignmentToCaps(GstCaps *caps) const
    {
        static const std::unordered_map<firebolt::rialto::SegmentAlignment, std::string> aligmentMap =
            {{firebolt::rialto::SegmentAlignment::AU, "au"}, {firebolt::rialto::SegmentAlignment::NAL, "nal"}};

        auto aligmentMapIt = aligmentMap.find(m_attachedSource.getSegmentAlignment());
        if (aligmentMapIt != aligmentMap.end())
        {
            m_gstWrapper->gstCapsSetSimple(caps, "alignment", G_TYPE_STRING, aligmentMapIt->second.c_str(), nullptr);
        }
    }

    void addCodecDataToCaps(GstCaps *caps) const
    {
        const std::vector<uint8_t> &codecData = m_attachedSource.getCodecData();
        if (!codecData.empty())
        {
            gpointer memory = m_glibWrapper->gMemdup(codecData.data(), codecData.size());
            GstBuffer *buf = m_gstWrapper->gstBufferNewWrapped(memory, codecData.size());
            m_gstWrapper->gstCapsSetSimple(caps, "codec_data", GST_TYPE_BUFFER, buf, nullptr);
            m_gstWrapper->gstBufferUnref(buf);
        }
    }

    void addStreamFormatToCaps(GstCaps *caps) const
    {
        static const std::unordered_map<firebolt::rialto::StreamFormat, std::string> formatMap =
            {{firebolt::rialto::StreamFormat::RAW, "raw"},
             {firebolt::rialto::StreamFormat::AVC, "avc"},
             {firebolt::rialto::StreamFormat::BYTE_STREAM, "byte-stream"}};

        auto formatMapIt = formatMap.find(m_attachedSource.getStreamFormat());
        if (formatMapIt != formatMap.end())
        {
            m_gstWrapper->gstCapsSetSimple(caps, "stream-format", G_TYPE_STRING, formatMapIt->second.c_str(), nullptr);
        }
    }

    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::shared_ptr<IGlibWrapper> m_glibWrapper;
    const IMediaPipeline::MediaSource &m_attachedSource;
};

class MediaSourceAudioCapsBuilder : public MediaSourceCapsBuilder
{
public:
    MediaSourceAudioCapsBuilder(std::shared_ptr<IGstWrapper> gstWrapper, std::shared_ptr<IGlibWrapper> glibWrapper,
                                const IMediaPipeline::MediaSourceAudio &source)
        : MediaSourceCapsBuilder(gstWrapper, glibWrapper, source), m_attachedAudioSource(source)
    {
    }

    GstCaps *buildCaps() override
    {
        std::string mimeType = m_attachedSource.getMimeType();
        if (mimeType == "audio/x-opus")
        {
            return createOpusCaps();
        }

        GstCaps *caps = buildCommonCaps();
        if (mimeType == "audio/mp4" || mimeType == "audio/aac")
        {
            addMpegVersionToCaps(caps);
        }
        addSampleRateAndChannelsToCaps(caps);

        return caps;
    }

protected:
    GstCaps *createOpusCaps()
    {
        GstCaps *caps = getAudioSpecificConfiguration();
        if (!caps)
        {
            caps = m_gstWrapper->gstCapsNewSimple("audio/x-opus", "channel-mapping-family", G_TYPE_INT, 0, nullptr);
            addSampleRateAndChannelsToCaps(caps);
        }

        return caps;
    }

    GstCaps *getAudioSpecificConfiguration() const
    {
        GstCaps *caps = nullptr;
        firebolt::rialto::AudioConfig audioConfig = m_attachedAudioSource.getAudioConfig();
        if (audioConfig.codecSpecificConfig.size())
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

    void addSampleRateAndChannelsToCaps(GstCaps *caps) const
    {
        firebolt::rialto::AudioConfig audioConfig = m_attachedAudioSource.getAudioConfig();

        if (audioConfig.numberOfChannels != firebolt::rialto::kInvalidAudioChannels)
            m_gstWrapper->gstCapsSetSimple(caps, "channels", G_TYPE_INT, audioConfig.numberOfChannels, nullptr);

        if (audioConfig.sampleRate != firebolt::rialto::kInvalidAudioSampleRate)
            m_gstWrapper->gstCapsSetSimple(caps, "rate", G_TYPE_INT, audioConfig.sampleRate, nullptr);
    }

    void addMpegVersionToCaps(GstCaps *caps) const
    {
        m_gstWrapper->gstCapsSetSimple(caps, "mpegversion", G_TYPE_INT, 4, nullptr);
    }

    const IMediaPipeline::MediaSourceAudio &m_attachedAudioSource;
};

class MediaSourceVideoDolbyVisionCapsBuilder : public MediaSourceCapsBuilder
{
public:
    MediaSourceVideoDolbyVisionCapsBuilder(std::shared_ptr<IGstWrapper> gstWrapper,
                                           std::shared_ptr<IGlibWrapper> glibWrapper,
                                           const IMediaPipeline::MediaSourceVideoDolbyVision &source)
        : MediaSourceCapsBuilder(gstWrapper, glibWrapper, source), m_attachedDolbySource(source)
    {
    }

    GstCaps *buildCaps() override
    {
        GstCaps *caps = buildCommonCaps();
        m_gstWrapper->gstCapsSetSimple(caps, "dovi-stream", G_TYPE_BOOLEAN, true, nullptr);
        m_gstWrapper->gstCapsSetSimple(caps, "dv_profile", G_TYPE_INT, m_attachedDolbySource.getDolbyVisionProfile(),
                                       nullptr);
        return caps;
    }

protected:
    const IMediaPipeline::MediaSourceVideoDolbyVision &m_attachedDolbySource;
};
}; // namespace

AttachSource::AttachSource(PlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper,
                           std::shared_ptr<IGlibWrapper> glibWrapper,
                           const std::unique_ptr<IMediaPipeline::MediaSource> &source)
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

GstCaps *AttachSource::createCapsFromMediaSource() const
{
    std::unique_ptr<MediaSourceCapsBuilder> capsBuilder;

    firebolt::rialto::SourceConfigType configType = m_attachedSource->getConfigType();
    if (configType == firebolt::rialto::SourceConfigType::AUDIO_BASE)
    {
        const IMediaPipeline::MediaSourceAudio &source =
            dynamic_cast<IMediaPipeline::MediaSourceAudio &>(*m_attachedSource);

        capsBuilder = std::make_unique<MediaSourceAudioCapsBuilder>(m_gstWrapper, m_glibWrapper, source);
    }
    else if (configType == firebolt::rialto::SourceConfigType::VIDEO_BASE)
    {
        capsBuilder = std::make_unique<MediaSourceCapsBuilder>(m_gstWrapper, m_glibWrapper, *m_attachedSource);
    }
    else if (configType == firebolt::rialto::SourceConfigType::VIDEO_DOLBY_VISION)
    {
        const IMediaPipeline::MediaSourceVideoDolbyVision &source =
            dynamic_cast<IMediaPipeline::MediaSourceVideoDolbyVision &>(*m_attachedSource);

        capsBuilder = std::make_unique<MediaSourceVideoDolbyVisionCapsBuilder>(m_gstWrapper, m_glibWrapper, source);
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("Invalid config type %u", static_cast<uint32_t>(configType));
        return nullptr;
    }

    return capsBuilder->buildCaps();
}
} // namespace firebolt::rialto::server
