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

#ifndef FIREBOLT_RIALTO_WRAPPERS_RDK_GSTREAMER_UTILS_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_RDK_GSTREAMER_UTILS_WRAPPER_H_

#include "IRdkGstreamerUtilsWrapper.h"
#include <memory>

namespace firebolt::rialto::wrappers
{
class RdkGstreamerUtilsWrapperFactory : public IRdkGstreamerUtilsWrapperFactory
{
public:
    std::shared_ptr<IRdkGstreamerUtilsWrapper> createRdkGstreamerUtilsWrapper() const override;
};

class RdkGstreamerUtilsWrapper : public IRdkGstreamerUtilsWrapper
{
public:
    RdkGstreamerUtilsWrapper() = default;
    ~RdkGstreamerUtilsWrapper() override = default;
    bool performAudioTrackCodecChannelSwitch(
        PlaybackGroupPrivate *playbackGroup, const void *sampleAttr, AudioAttributesPrivate *audioAttr,
        std::uint32_t *status, unsigned int *ui32Delay, long long *audioChangeTargetPts, // NOLINT(runtime/int)
        const long long *currentDispPts, unsigned int *audioChangeStage,                 // NOLINT(runtime/int)
        GstCaps **appsrcCaps, bool *audioaac, bool svpEnabled, GstElement *aSrc, bool *ret) const override;
    void processAudioGap(GstElement *pipeline, gint64 gapstartpts, gint32 gapduration, gint64 gapdiscontinuity,
                         bool audioaac) const override;
    void doAudioEasingonSoc(double targetVolume, uint32_t volumeDuration, rgu_Ease easeType) const override;
    bool initialVolSettingNeeded() const override;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_RDK_GSTREAMER_UTILS_WRAPPER_H_
