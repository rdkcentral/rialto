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
} // namespace firebolt::rialto::wrappers
