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

#include "rdk_gstreamer_utils.h"
namespace rdk_gstreamer_utils
{
bool performAudioTrackCodecChannelSwitch(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup,
                                         const void *pSampleAttr, AudioAttributes *pAudioAttr, uint32_t *pStatus,
                                         unsigned int *pui32Delay, long long *pAudioChangeTargetPts,
                                         const long long *pcurrentDispPts, unsigned int *audio_change_stage,
                                         GstCaps **appsrcCaps, bool *audioaac, bool svpenabled, GstElement *aSrc,
                                         bool *ret)
{
    return true;
}

void processAudioGap(GstElement *pipeline, gint64 gapstartpts, gint32 gapduration, gint64 gapdiscontinuity, bool audioaac)
{
}

void doAudioEasingonSoc(double targetVolume, uint32_t volumeDuration, rdk_gstreamer_utils::rgu_Ease easeType) {}

bool isSocAudioFadeSupported()
{
    return true;
}
} // namespace rdk_gstreamer_utils
