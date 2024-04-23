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

#include "tasks/generic/AttachSource.h"
#include "GstMimeMapping.h"
#include "IGlibWrapper.h"
#include "IGstWrapper.h"
#include "IMediaPipeline.h"
#include "RialtoServerLogging.h"
#include <optional>
#include <unordered_map>

namespace firebolt::rialto::server::tasks::generic
{
namespace
{
class MediaSourceCapsBuilder
{
public:
    MediaSourceCapsBuilder(std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
                           std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper,
                           const firebolt::rialto::IMediaPipeline::MediaSourceAV &source)
        : m_gstWrapper(gstWrapper), m_glibWrapper(glibWrapper), m_attachedSource(source)
    {
    }
    virtual GstCaps *buildCaps() { return buildCommonCaps(); }

protected:
    std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> m_gstWrapper;
    std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> m_glibWrapper;
    const IMediaPipeline::MediaSourceAV &m_attachedSource;

    GstCaps *buildCommonCaps()
    {
        GstCaps *caps = firebolt::rialto::server::createSimpleCapsFromMimeType(m_gstWrapper, m_attachedSource);

        addAlignmentToCaps(caps);
        addCodecDataToCaps(caps);
        addStreamFormatToCaps(caps);

        return caps;
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
        const std::shared_ptr<CodecData> &kCodecData = m_attachedSource.getCodecData();
        if (kCodecData && CodecDataType::BUFFER == kCodecData->type)
        {
            gpointer memory = m_glibWrapper->gMemdup(kCodecData->data.data(), kCodecData->data.size());
            GstBuffer *buf = m_gstWrapper->gstBufferNewWrapped(memory, kCodecData->data.size());
            m_gstWrapper->gstCapsSetSimple(caps, "codec_data", GST_TYPE_BUFFER, buf, nullptr);
            m_gstWrapper->gstBufferUnref(buf);
        }
        else if (kCodecData && CodecDataType::STRING == kCodecData->type)
        {
            std::string codecDataStr{kCodecData->data.begin(), kCodecData->data.end()};
            m_gstWrapper->gstCapsSetSimple(caps, "codec_data", G_TYPE_STRING, codecDataStr.c_str(), nullptr);
        }
    }

    void addStreamFormatToCaps(GstCaps *caps) const
    {
        static const std::unordered_map<firebolt::rialto::StreamFormat, std::string> formatMap =
            {{firebolt::rialto::StreamFormat::RAW, "raw"},
             {firebolt::rialto::StreamFormat::AVC, "avc"},
             {firebolt::rialto::StreamFormat::BYTE_STREAM, "byte-stream"},
             {firebolt::rialto::StreamFormat::HVC1, "hvc1"},
             {firebolt::rialto::StreamFormat::HEV1, "hev1"}};

        auto formatMapIt = formatMap.find(m_attachedSource.getStreamFormat());
        if (formatMapIt != formatMap.end())
        {
            m_gstWrapper->gstCapsSetSimple(caps, "stream-format", G_TYPE_STRING, formatMapIt->second.c_str(), nullptr);
        }
    }
};

class MediaSourceAudioCapsBuilder : public MediaSourceCapsBuilder
{
public:
    MediaSourceAudioCapsBuilder(std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
                                std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper,
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

        GstCaps *caps = MediaSourceCapsBuilder::buildCaps();
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

class MediaSourceVideoCapsBuilder : public MediaSourceCapsBuilder
{
public:
    MediaSourceVideoCapsBuilder(std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
                                std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper,
                                const IMediaPipeline::MediaSourceVideo &source)
        : MediaSourceCapsBuilder(gstWrapper, glibWrapper, source), m_attachedVideoSource(source)
    {
    }

    GstCaps *buildCaps() override
    {
        GstCaps *caps = MediaSourceCapsBuilder::buildCaps();
        if (m_attachedVideoSource.getWidth() != firebolt::rialto::kUndefinedSize)
            m_gstWrapper->gstCapsSetSimple(caps, "width", G_TYPE_INT, m_attachedVideoSource.getWidth(), nullptr);
        if (m_attachedVideoSource.getHeight() != firebolt::rialto::kUndefinedSize)
            m_gstWrapper->gstCapsSetSimple(caps, "height", G_TYPE_INT, m_attachedVideoSource.getHeight(), nullptr);

        return caps;
    }

protected:
    const IMediaPipeline::MediaSourceVideo &m_attachedVideoSource;
};

class MediaSourceVideoDolbyVisionCapsBuilder : public MediaSourceVideoCapsBuilder
{
public:
    MediaSourceVideoDolbyVisionCapsBuilder(std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
                                           std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper,
                                           const IMediaPipeline::MediaSourceVideoDolbyVision &source)
        : MediaSourceVideoCapsBuilder(gstWrapper, glibWrapper, source), m_attachedDolbySource(source)
    {
    }

    GstCaps *buildCaps() override
    {
        GstCaps *caps = MediaSourceVideoCapsBuilder::buildCaps();
        m_gstWrapper->gstCapsSetSimple(caps, "dovi-stream", G_TYPE_BOOLEAN, true, nullptr);
        m_gstWrapper->gstCapsSetSimple(caps, "dv_profile", G_TYPE_UINT, m_attachedDolbySource.getDolbyVisionProfile(),
                                       nullptr);
        return caps;
    }

protected:
    const IMediaPipeline::MediaSourceVideoDolbyVision &m_attachedDolbySource;
};
}; // namespace

AttachSource::AttachSource(
    GenericPlayerContext &context, std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
    std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper,
    IGstGenericPlayerPrivate &player, const std::unique_ptr<IMediaPipeline::MediaSource> &source)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper},
      m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper}, m_player{player}, m_attachedSource{source->copy()}
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
    else if (m_attachedSource->getType() == MediaSourceType::SUBTITLE)
    {
        // just stub for now
        RIALTO_SERVER_LOG_DEBUG("Subtitle source attached");
        return;
    }

    GstCaps *caps = createCapsFromMediaSource();
    gchar *capsStr = m_gstWrapper->gstCapsToString(caps);
    std::string strCaps = capsStr;
    m_glibWrapper->gFree(capsStr);

    RIALTO_SERVER_LOG_DEBUG("caps str: '%s'", strCaps.c_str());

    if (m_context.streamInfo.find(m_attachedSource->getType()) == m_context.streamInfo.end())
    {
        addSource(caps, m_attachedSource->getHasDrm());
    }
    else if (m_attachedSource->getType() == MediaSourceType::AUDIO && m_context.audioSourceRemoved)
    {
        reattachAudioSource(caps, strCaps);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("cannot update caps");
    }

    if (caps)
        m_gstWrapper->gstCapsUnref(caps);
}

void AttachSource::addSource(GstCaps *caps, bool hasDrm) const
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
    m_context.streamInfo.emplace(m_attachedSource->getType(), StreamInfo{appSrc, hasDrm});
}

void AttachSource::reattachAudioSource(GstCaps *caps, const std::string &strCaps) const
{
    if (m_attachedSource->getMimeType().empty())
    {
        RIALTO_SERVER_LOG_WARN("SKIP reattach audio source. Unknown mime type");
        return;
    }

    std::int64_t currentDispPts64b; // In netflix code it's currentDisplayPosition + offset
    m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &currentDispPts64b);
    long long currentDispPts = currentDispPts64b; // NOLINT(runtime/int)

    GstAppSrc *appSrc{GST_APP_SRC(m_context.streamInfo[m_attachedSource->getType()].appSrc)};
    GstCaps *oldCaps = m_gstWrapper->gstAppSrcGetCaps(appSrc);
    if ((!oldCaps) || (!m_gstWrapper->gstCapsIsEqual(caps, oldCaps)))
    {
        RIALTO_SERVER_LOG_MIL("Switching audio source.");

        gchar *oldCapsCStr = m_gstWrapper->gstCapsToString(oldCaps);
        std::string oldCapsStr = std::string(oldCapsCStr);
        m_glibWrapper->gFree(oldCapsCStr);

        RIALTO_SERVER_LOG_MIL("Old caps: %s", oldCapsStr.c_str());
        RIALTO_SERVER_LOG_MIL("New caps: %s", strCaps.c_str());

        int sampleAttributes{
            0}; // rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch checks if this param != NULL only.
        std::uint32_t status{0};   // must be 0 to make rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch work
        unsigned int ui32Delay{0}; // output param
        long long audioChangeTargetPts{-1}; // NOLINT(runtime/int) output param. Set audioChangeTargetPts =
                                            // currentDispPts in rdk_gstreamer_utils function stub
        unsigned int audioChangeStage{0};   // Output param. Set to AUDCHG_ALIGN in rdk_gstreamer_utils function stub
        bool audioAac{oldCapsStr.find("audio/mpeg") != std::string::npos};
        bool svpEnabled{true}; // assume always true
        bool retVal{false};    // Output param. Set to TRUE in rdk_gstreamer_utils function stub

        std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> audioAttributes = createAudioAttributes();
        if (!audioAttributes)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create audio attributes");
            return;
        }

        bool result =
            m_rdkGstreamerUtilsWrapper
                ->performAudioTrackCodecChannelSwitch(&m_context.playbackGroup, &sampleAttributes, &(*audioAttributes),
                                                      &status, &ui32Delay, &audioChangeTargetPts, &currentDispPts,
                                                      &audioChangeStage,
                                                      &caps, // may fail for amlogic - that implementation changes
                                                             // this parameter, it's probably used by Netflix later
                                                      &audioAac, svpEnabled, GST_ELEMENT(appSrc), &retVal);

        if (!result || !retVal)
        {
            RIALTO_SERVER_LOG_WARN("performAudioTrackCodecChannelSwitch failed! Result: %d, retval %d", result, retVal);
        }
    }
    else
    {
        RIALTO_SERVER_LOG_MIL("Reattaching identical audio source.");
    }

    // Restart audio sink
    m_player.setAudioVideoFlags(true, true);

    m_context.audioNeedData = true;
    m_context.audioSourceRemoved = false;
    m_context.lastAudioSampleTimestamps = currentDispPts;
    m_player.notifyNeedMediaData(true, false);

    if (oldCaps)
        m_gstWrapper->gstCapsUnref(oldCaps);
}

GstCaps *AttachSource::createCapsFromMediaSource() const
{
    std::unique_ptr<MediaSourceCapsBuilder> capsBuilder;

    firebolt::rialto::SourceConfigType configType = m_attachedSource->getConfigType();
    if (configType == firebolt::rialto::SourceConfigType::AUDIO)
    {
        const IMediaPipeline::MediaSourceAudio *kSource =
            dynamic_cast<IMediaPipeline::MediaSourceAudio *>(m_attachedSource.get());
        if (kSource)
        {
            capsBuilder = std::make_unique<MediaSourceAudioCapsBuilder>(m_gstWrapper, m_glibWrapper, *kSource);
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to cast to audio source");
            return nullptr;
        }
    }
    else if (configType == firebolt::rialto::SourceConfigType::VIDEO)
    {
        const IMediaPipeline::MediaSourceVideo *kSource =
            dynamic_cast<IMediaPipeline::MediaSourceVideo *>(m_attachedSource.get());
        if (kSource)
        {
            capsBuilder = std::make_unique<MediaSourceVideoCapsBuilder>(m_gstWrapper, m_glibWrapper, *kSource);
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to cast to video source");
            return nullptr;
        }
    }
    else if (configType == firebolt::rialto::SourceConfigType::VIDEO_DOLBY_VISION)
    {
        const IMediaPipeline::MediaSourceVideoDolbyVision *kSource =
            dynamic_cast<IMediaPipeline::MediaSourceVideoDolbyVision *>(m_attachedSource.get());
        if (kSource)
        {
            capsBuilder = std::make_unique<MediaSourceVideoDolbyVisionCapsBuilder>(m_gstWrapper, m_glibWrapper, *kSource);
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to cast to dolby vision source");
            return nullptr;
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Invalid config type %u", static_cast<uint32_t>(configType));
        return nullptr;
    }

    return capsBuilder->buildCaps();
}

std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> AttachSource::createAudioAttributes() const
{
    const IMediaPipeline::MediaSourceAudio *kSource =
        dynamic_cast<IMediaPipeline::MediaSourceAudio *>(m_attachedSource.get());
    if (kSource)
    {
        firebolt::rialto::AudioConfig audioConfig = kSource->getAudioConfig();
        firebolt::rialto::wrappers::AudioAttributesPrivate
            audioAttributes{"", // param set below.
                            audioConfig.numberOfChannels, audioConfig.sampleRate,
                            0, // used only in one of logs in rdk_gstreamer_utils, no
                               // need to set this param.
                            0, // used only in one of logs in rdk_gstreamer_utils, no
                               // need to set this param.
                            audioConfig.codecSpecificConfig.data(),
                            static_cast<std::uint32_t>(audioConfig.codecSpecificConfig.size())};
        if (m_attachedSource->getMimeType() == "audio/mp4" || m_attachedSource->getMimeType() == "audio/aac")
        {
            audioAttributes.m_codecParam = "mp4a.40.2, mp4a.40.5";
        }
        else if (m_attachedSource->getMimeType() == "audio/x-eac3")
        {
            audioAttributes.m_codecParam = std::string("ec-3.A") + std::to_string(audioConfig.numberOfChannels);
        }
        return audioAttributes ;
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to cast to dolby vision source");
        return std::nullopt;
    }
}
} // namespace firebolt::rialto::server::tasks::generic
