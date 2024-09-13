/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "RdkGstreamerUtilsWrapper.h"
#include "rdk_gstreamer_utils.h"
#include <stdexcept>

namespace firebolt::rialto::wrappers
{
std::shared_ptr<IRdkGstreamerUtilsWrapper> RdkGstreamerUtilsWrapperFactory::createRdkGstreamerUtilsWrapper() const
{
    return std::make_shared<RdkGstreamerUtilsWrapper>();
}

bool RdkGstreamerUtilsWrapper::performAudioTrackCodecChannelSwitch(
    PlaybackGroupPrivate *playbackGroup, const void *sampleAttr, AudioAttributesPrivate *audioAttr, std::uint32_t *status,
    unsigned int *ui32Delay, long long *audioChangeTargetPts, const long long *currentDispPts, // NOLINT(runtime/int)
    unsigned int *audioChangeStage, GstCaps **appsrcCaps, bool *audioaac, bool svpEnabled, GstElement *aSrc,
    bool *ret) const
{
    if (!playbackGroup || !audioAttr)
    {
        return false;
    }
    rdk_gstreamer_utils::rdkGstreamerUtilsPlaybackGrp *rdkPlaybackGroup{
        reinterpret_cast<rdk_gstreamer_utils::rdkGstreamerUtilsPlaybackGrp *>(playbackGroup)};
    rdk_gstreamer_utils::AudioAttributes *rdkAudioAttributes{
        reinterpret_cast<rdk_gstreamer_utils::AudioAttributes *>(audioAttr)};
    return rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch(rdkPlaybackGroup, sampleAttr, rdkAudioAttributes,
                                                                    status, ui32Delay, audioChangeTargetPts,
                                                                    currentDispPts, audioChangeStage, appsrcCaps,
                                                                    audioaac, svpEnabled, aSrc, ret);
}

void RdkGstreamerUtilsWrapper::processAudioGap(GstElement *pipeline, gint64 gapstartpts, gint32 gapduration,
                                               gint64 gapdiscontinuity, bool audioaac) const
{
    return rdk_gstreamer_utils::processAudioGap(pipeline, gapstartpts, gapduration, gapdiscontinuity, audioaac);
}

rdk_gstreamer_utils::rgu_Ease convertEaseType(firebolt::rialto::wrappers::EaseType ease)
{
    switch (ease)
    {
    case firebolt::rialto::wrappers::EaseType::EASE_LINEAR:
        return rdk_gstreamer_utils::rgu_Ease::EaseLinear;
    case firebolt::rialto::wrappers::EaseType::EASE_IN_CUBIC:
        return rdk_gstreamer_utils::rgu_Ease::EaseInCubic;
    case firebolt::rialto::wrappers::EaseType::EASE_OUT_CUBIC:
        return rdk_gstreamer_utils::rgu_Ease::EaseOutCubic;
    default:
        throw std::invalid_argument("Unknown EaseType");
    }
}

void RdkGstreamerUtilsWrapper::doAudioEasingonSoc(double target, uint32_t duration, rgu_Ease ease) const
{
    rdk_gstreamer_utils::rgu_Ease convertedEaseType =
        convertEaseType(static_cast<firebolt::rialto::wrappers::EaseType>(ease));
    return rdk_gstreamer_utils::doAudioEasingonSoc(target, duration, convertedEaseType);
}

bool RdkGstreamerUtilsWrapper::isSocAudioFadeSupported() const
{
    return rdk_gstreamer_utils::isSocAudioFadeSupported();
}
} // namespace firebolt::rialto::wrappers
