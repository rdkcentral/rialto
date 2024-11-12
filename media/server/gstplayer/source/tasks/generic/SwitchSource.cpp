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

namespace firebolt::rialto::server::tasks::generic
{
SwitchSource::SwitchSource(
    GenericPlayerContext &context, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper},
      m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper}
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
    int sampleAttributes{0}; // rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch checks if this param != NULL only.
    std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> audioAttributes{
        std::nullopt};         // UZUPELNIJ TO, TRZEBA BEDZIE PRZEKAZAC MEDIASOURCE DO TEJ METODY JAK W ATTACH SOURCE
    std::uint32_t status{0};   // must be 0 to make rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch work
    unsigned int ui32Delay{0}; // output param
    long long audioChangeTargetPts{-1}; // NOLINT(runtime/int) output param. Set audioChangeTargetPts =
                                        // currentDispPts in rdk_gstreamer_utils function stub
    std::int64_t currentDispPts64b;     // In netflix code it's currentDisplayPosition + offset
    m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &currentDispPts64b);
    long long currentDispPts = currentDispPts64b; // NOLINT(runtime/int)
    unsigned int audioChangeStage{0}; // Output param. Set to AUDCHG_ALIGN in rdk_gstreamer_utils function stub
    GstCaps *caps =
        nullptr; // UZUPELNIJ TO createCapsFromMediaSource() w attach source, pewnie trzeba przeniesc ten generator do commona
    GstAppSrc *appSrc{nullptr}; // UZUPELNIJ TO {GST_APP_SRC(m_context.streamInfo[m_attachedSource->getType()].appSrc)};
                                // wiec wziac z MEDIASOURCE typ i sobie to sciagnac
    GstCaps *oldCaps = m_gstWrapper->gstAppSrcGetCaps(appSrc);
    gchar *oldCapsCStr = m_gstWrapper->gstCapsToString(oldCaps);
    std::string oldCapsStr = std::string(oldCapsCStr);
    m_glibWrapper->gFree(oldCapsCStr);
    bool audioAac{oldCapsStr.find("audio/mpeg") != std::string::npos};
    bool svpEnabled{true}; // assume always true
    bool retVal{false};    // Output param. Set to TRUE in rdk_gstreamer_utils function stub
    bool result = m_rdkGstreamerUtilsWrapper
                      ->performAudioTrackCodecChannelSwitch(&m_context.playbackGroup, &sampleAttributes,
                                                            &(*audioAttributes), &status, &ui32Delay,
                                                            &audioChangeTargetPts, &currentDispPts, &audioChangeStage,
                                                            &caps, // may fail for amlogic - that implementation changes
                                                                   // this parameter, it's probably used by Netflix later
                                                            &audioAac, svpEnabled, GST_ELEMENT(appSrc), &retVal);

    if (!result || !retVal)
    {
        RIALTO_SERVER_LOG_WARN("performAudioTrackCodecChannelSwitch failed! Result: %d, retval %d", result, retVal);
    }
}
} // namespace firebolt::rialto::server::tasks::generic
