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

#ifndef FIREBOLT_RIALTO_SERVER_I_RDK_GSTREAMER_UTILS_WRAPPER_H_
#define FIREBOLT_RIALTO_SERVER_I_RDK_GSTREAMER_UTILS_WRAPPER_H_

#include <cstdint>
#include <gst/gst.h>
#include <memory>
#include <string>

namespace firebolt::rialto::server
{
struct PlaybackGroupPrivate
{
    GstElement *m_gstPipeline;
    GstElement *m_curAudioPlaysinkBin;
    GstElement *m_curAudioDecodeBin;
    GstElement *m_curAudioDecoder;
    GstElement *m_curAudioParse;
    GstElement *m_curAudioTypefind;
    bool m_linkTypefindParser;
    bool m_isAudioAAC;
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

    virtual bool performAudioTrackCodecChannelSwitch(PlaybackGroupPrivate *gstUtilsPlaybackGroup, const void *sampleAttr,
                                                     AudioAttributesPrivate *audioAttr, std::uint32_t *status,
                                                     unsigned int *ui32Delay, long long *audioChangeTargetPts,
                                                     const long long *currentDispPts, unsigned int *audioChangeStage,
                                                     GstCaps **appsrcCaps, bool *audioaac, bool svpEnabled,
                                                     GstElement *aSrc, bool *ret) const = 0;
    virtual void deepElementAdded(PlaybackGroupPrivate *pgstUtilsPlaybackGroup, GstBin *pipeline, GstBin *bin,
                                  GstElement *element) = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_RDK_GSTREAMER_UTILS_WRAPPER_H_
