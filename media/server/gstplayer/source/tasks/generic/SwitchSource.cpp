/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include "SwitchSource.h"
#include "RialtoServerLogging.h"
#include "Utils.h"

namespace firebolt::rialto::server::tasks::generic
{
SwitchSource::SwitchSource(
    GenericPlayerContext &context, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper,
    const std::unique_ptr<IMediaPipeline::MediaSource> &source)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper},
      m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper}, m_source{source->copy()}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SwitchSource");
}

SwitchSource::~SwitchSource()
{
    RIALTO_SERVER_LOG_DEBUG("SwitchSource finished");
}

void SwitchSource::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SwitchSource");
    if (m_context.streamInfo.find(m_source->getType()) == m_context.streamInfo.end())
    {
        RIALTO_SERVER_LOG_ERROR("Unable to switch source, type does not exist");
        return;
    }
    if (m_source->getMimeType().empty())
    {
        RIALTO_SERVER_LOG_WARN("Skip switch audio source. Unknown mime type");
        return;
    }
    std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> audioAttributes{createAudioAttributes()};
    if (!audioAttributes)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create audio attributes");
        return;
    }
    std::int64_t currentDispPts64b;     // In netflix code it's currentDisplayPosition + offset
    m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &currentDispPts64b);
    long long currentDispPts = currentDispPts64b; // NOLINT(runtime/int)
    GstCaps *caps{createCapsFromMediaSource(m_gstWrapper, m_glibWrapper, m_source)};
    GstAppSrc *appSrc{GST_APP_SRC(m_context.streamInfo[m_source->getType()].appSrc)};
    GstCaps *oldCaps = m_gstWrapper->gstAppSrcGetCaps(appSrc);
    if ((!oldCaps) || (!m_gstWrapper->gstCapsIsEqual(caps, oldCaps)))
    {
        RIALTO_SERVER_LOG_DEBUG("Caps not equal. Perform audio track codec channel switch.");
        int sampleAttributes{
            0}; // rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch checks if this param != NULL only.
        std::uint32_t status{0};   // must be 0 to make rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch work
        unsigned int ui32Delay{0}; // output param
        long long audioChangeTargetPts{-1}; // NOLINT(runtime/int) output param. Set audioChangeTargetPts =
                                            // currentDispPts in rdk_gstreamer_utils function stub
        unsigned int audioChangeStage{0};   // Output param. Set to AUDCHG_ALIGN in rdk_gstreamer_utils function stub
        gchar *oldCapsCStr = m_gstWrapper->gstCapsToString(oldCaps);
        std::string oldCapsStr = std::string(oldCapsCStr);
        m_glibWrapper->gFree(oldCapsCStr);
        bool audioAac{oldCapsStr.find("audio/mpeg") != std::string::npos};
        bool svpEnabled{true}; // assume always true
        bool retVal{false};    // Output param. Set to TRUE in rdk_gstreamer_utils function stub
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
        RIALTO_SERVER_LOG_DEBUG("Skip switching audio source - caps are the same.");
    }

    m_context.lastAudioSampleTimestamps = currentDispPts;
    if (caps)
        m_gstWrapper->gstCapsUnref(caps);
    if (oldCaps)
        m_gstWrapper->gstCapsUnref(oldCaps);
}

std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> SwitchSource::createAudioAttributes() const
{
    std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> audioAttributes;
    const IMediaPipeline::MediaSourceAudio *kSource = dynamic_cast<IMediaPipeline::MediaSourceAudio *>(m_source.get());
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
        if (m_source->getMimeType() == "audio/mp4" || m_source->getMimeType() == "audio/aac")
        {
            audioAttributes->m_codecParam = "mp4a.40.2, mp4a.40.5";
        }
        else if (m_source->getMimeType() == "audio/x-eac3")
        {
            audioAttributes->m_codecParam = std::string("ec-3.A") + std::to_string(audioConfig.numberOfChannels);
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to cast source");
    }

    return audioAttributes;
}
} // namespace firebolt::rialto::server::tasks::generic
