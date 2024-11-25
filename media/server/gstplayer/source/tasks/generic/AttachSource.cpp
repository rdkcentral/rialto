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
#include "TypeConverters.h"
#include "Utils.h"
#include <unordered_map>

namespace firebolt::rialto::server::tasks::generic
{
AttachSource::AttachSource(
    GenericPlayerContext &context, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> &rdkGstreamerUtilsWrapper,
    const std::shared_ptr<IGstTextTrackSinkFactory> &gstTextTrackSinkFactory, IGstGenericPlayerPrivate &player,
    const std::unique_ptr<IMediaPipeline::MediaSource> &source)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper},
      m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper},
      m_gstTextTrackSinkFactory{gstTextTrackSinkFactory}, m_player{player}, m_attachedSource{source->copy()}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing AttachSource");
}

AttachSource::~AttachSource()
{
    RIALTO_SERVER_LOG_DEBUG("AttachSource finished");
}

void AttachSource::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing AttachSource %u", static_cast<uint32_t>(m_attachedSource->getType()));

    if (m_attachedSource->getType() == MediaSourceType::UNKNOWN)
    {
        RIALTO_SERVER_LOG_ERROR("Unknown media source type");
        return;
    }

    GstCaps *caps = createCapsFromMediaSource(m_gstWrapper, m_glibWrapper, m_attachedSource);
    if (!caps)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create caps from media source");
        return;
    }
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
    else if (m_attachedSource->getType() == MediaSourceType::SUBTITLE)
    {
        RIALTO_SERVER_LOG_MIL("Adding Subtitle appsrc");
        appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "subsrc");

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(m_context.pipeline), "text-sink"))
        {
            GstElement *elem = m_gstTextTrackSinkFactory->createGstTextTrackSink();
            m_context.subtitleSink = elem;

            m_glibWrapper->gObjectSet(m_context.pipeline, "text-sink", elem, nullptr);
        }
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
    m_player.setPlaybinFlags(true);

    m_context.streamInfo[m_attachedSource->getType()].isDataNeeded = true;
    m_context.audioSourceRemoved = false;
    m_context.lastAudioSampleTimestamps = currentDispPts;
    m_player.notifyNeedMediaData(MediaSourceType::AUDIO);

    if (oldCaps)
        m_gstWrapper->gstCapsUnref(oldCaps);
}

std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> AttachSource::createAudioAttributes() const
{
    std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> audioAttributes;
    const IMediaPipeline::MediaSourceAudio *kSource =
        dynamic_cast<IMediaPipeline::MediaSourceAudio *>(m_attachedSource.get());
    if (kSource)
    {
        firebolt::rialto::AudioConfig audioConfig = kSource->getAudioConfig();
        audioAttributes =
            firebolt::rialto::wrappers::AudioAttributesPrivate{"", // param set below.
                                                               audioConfig.numberOfChannels, audioConfig.sampleRate,
                                                               0, // used only in one of logs in rdk_gstreamer_utils, no
                                                                  // need to set this param.
                                                               0, // used only in one of logs in rdk_gstreamer_utils, no
                                                                  // need to set this param.
                                                               audioConfig.codecSpecificConfig.data(),
                                                               static_cast<std::uint32_t>(
                                                                   audioConfig.codecSpecificConfig.size())};
        if (m_attachedSource->getMimeType() == "audio/mp4" || m_attachedSource->getMimeType() == "audio/aac")
        {
            audioAttributes->m_codecParam = "mp4a.40.2, mp4a.40.5";
        }
        else if (m_attachedSource->getMimeType() == "audio/x-eac3")
        {
            audioAttributes->m_codecParam = std::string("ec-3.A") + std::to_string(audioConfig.numberOfChannels);
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to cast to dolby vision source");
    }

    return audioAttributes;
}
} // namespace firebolt::rialto::server::tasks::generic
