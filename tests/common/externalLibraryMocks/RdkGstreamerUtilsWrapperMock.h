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

#ifndef FIREBOLT_RIALTO_WRAPPERS_RDK_GSTREAMER_UTILS_WRAPPER_MOCK_H_
#define FIREBOLT_RIALTO_WRAPPERS_RDK_GSTREAMER_UTILS_WRAPPER_MOCK_H_

#include "IRdkGstreamerUtilsWrapper.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::wrappers
{
class RdkGstreamerUtilsWrapperMock : public IRdkGstreamerUtilsWrapper
{
public:
    MOCK_METHOD(bool, performAudioTrackCodecChannelSwitch,
                (PlaybackGroupPrivate * playbackGroup, const void *sampleAttr, AudioAttributesPrivate *audioAttr,
                 std::uint32_t *status, unsigned int *ui32Delay, long long *audioChangeTargetPts, // NOLINT(runtime/int)
                 const long long *currentDispPts, unsigned int *audioChangeStage,                 // NOLINT(runtime/int)
                 GstCaps **appsrcCaps, bool *audioaac, bool svpEnabled, GstElement *aSrc, bool *ret),
                (const, override));
    MOCK_METHOD(void, processAudioGap,
                (GstElement * pipeline, gint64 gapstartpts, gint32 gapduration, gint64 gapdiscontinuity, bool audioaac),
                (const, override));
    MOCK_METHOD(void, doAudioEasingonSoc, (double targetVolume, uint32_t volumeDuration, rgu_Ease easeType), (const, override));
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_RDK_GSTREAMER_UTILS_WRAPPER_MOCK_H_
