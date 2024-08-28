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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_RDK_GSTREAMER_UTILS_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_RDK_GSTREAMER_UTILS_WRAPPER_H_

#include <cstdint>
#include <gst/gst.h>
#include <memory>
#include <string>

namespace firebolt::rialto::wrappers
{
struct PlaybackGroupPrivate
{
    GstElement *m_gstPipeline{nullptr};
    GstElement *m_curAudioPlaysinkBin{nullptr};
    GstElement *m_curAudioDecodeBin{nullptr};
    GstElement *m_curAudioDecoder{nullptr};
    GstElement *m_curAudioParse{nullptr};
    GstElement *m_curAudioTypefind{nullptr};
    bool m_linkTypefindParser{false};
    bool m_isAudioAAC{false};
};

struct AudioAttributesPrivate
{
    std::string m_codecParam{};
    std::uint32_t m_numberOfChannels{0};
    std::uint32_t m_samplesPerSecond{0};
    std::uint32_t m_bitrate{0};
    std::uint32_t m_blockAlignment{0};
    const std::uint8_t *m_codecSpecificData{nullptr};
    std::uint32_t m_codecSpecificDataLen{0};
};

enum rgu_Ease
{
    EaseLinear = 0,
    EaseInCubic,
    EaseOutCubic,
    EaseCount
};

class IRdkGstreamerUtilsWrapper;
class IRdkGstreamerUtilsWrapperFactory
{
public:
    IRdkGstreamerUtilsWrapperFactory() = default;
    virtual ~IRdkGstreamerUtilsWrapperFactory() = default;

    /**
     * @brief Gets the IRdkGstreamerUtilsWrapperFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> getFactory();

    /**
     * @brief Gets a IRdkGstreamerUtilsWrapper instance
     *
     * @retval the instance or null on error.
     */
    virtual std::shared_ptr<IRdkGstreamerUtilsWrapper> createRdkGstreamerUtilsWrapper() const = 0;
};

class IRdkGstreamerUtilsWrapper
{
public:
    IRdkGstreamerUtilsWrapper() = default;
    virtual ~IRdkGstreamerUtilsWrapper() = default;

    virtual bool performAudioTrackCodecChannelSwitch(
        PlaybackGroupPrivate *playbackGroup, const void *sampleAttr, AudioAttributesPrivate *audioAttr,
        std::uint32_t *status, unsigned int *ui32Delay, long long *audioChangeTargetPts, // NOLINT(runtime/int)
        const long long *currentDispPts, unsigned int *audioChangeStage,                 // NOLINT(runtime/int)
        GstCaps **appsrcCaps, bool *audioaac, bool svpEnabled, GstElement *aSrc, bool *ret) const = 0;
    virtual void processAudioGap(GstElement *pipeline, gint64 gapstartpts, gint32 gapduration, gint64 gapdiscontinuity,
                                 bool audioaac) const = 0;
    virtual void doAudioEasingonSoc(double targetVolume, uint32_t volumeDuration, rgu_Ease easeType) const = 0;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_RDK_GSTREAMER_UTILS_WRAPPER_H_
