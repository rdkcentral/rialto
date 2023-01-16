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
#include "RialtoServerLogging.h"
#include "rdk_gstreamer_utils.h"

namespace
{
rdk_gstreamer_utils::rdkGstreamerUtilsPlaybackGrp
convertPlaybackGroupPrivate(const firebolt::rialto::server::PlaybackGroupPrivate &playbackGroupPrivate)
{
    return rdk_gstreamer_utils::rdkGstreamerUtilsPlaybackGrp{playbackGroupPrivate.m_gstPipeline,
                                                             playbackGroupPrivate.m_curAudioPlaysinkBin,
                                                             playbackGroupPrivate.m_curAudioDecodeBin,
                                                             playbackGroupPrivate.m_curAudioDecoder,
                                                             playbackGroupPrivate.m_curAudioParse,
                                                             playbackGroupPrivate.m_curAudioTypefind,
                                                             playbackGroupPrivate.m_linkTypefindParser,
                                                             playbackGroupPrivate.m_isAudioAAC};
}

rdk_gstreamer_utils::AudioAttributes
convertAudioAttributesPrivate(const firebolt::rialto::server::AudioAttributesPrivate &audioAttributesPrivate)
{
    rdk_gstreamer_utils::AudioAttributes rdkAudioAttributes;
    rdkAudioAttributes.mCodecParam = audioAttributesPrivate.m_codecParam;
    rdkAudioAttributes.mNumberOfChannels = audioAttributesPrivate.m_numberOfChannels;
    rdkAudioAttributes.mSamplesPerSecond = audioAttributesPrivate.m_samplesPerSecond;
    rdkAudioAttributes.mBitrate = audioAttributesPrivate.m_bitrate;
    rdkAudioAttributes.mBlockAlignment = audioAttributesPrivate.m_blockAlignment;
    rdkAudioAttributes.mCodecSpecificData = audioAttributesPrivate.m_codecSpecificData;
    rdkAudioAttributes.mCodecSpecificDataLen = audioAttributesPrivate.m_codecSpecificDataLen;
    return rdkAudioAttributes;
}

void convertRdkPlaybackGroup(const rdk_gstreamer_utils::rdkGstreamerUtilsPlaybackGrp &rdkPlaybackGroup,
                             firebolt::rialto::server::PlaybackGroupPrivate &playbackGroupPrivate)
{
    playbackGroupPrivate.m_gstPipeline = rdkPlaybackGroup.gstPipeline;
    playbackGroupPrivate.m_curAudioPlaysinkBin = rdkPlaybackGroup.curAudioPlaysinkBin;
    playbackGroupPrivate.m_curAudioDecodeBin = rdkPlaybackGroup.curAudioDecodeBin;
    playbackGroupPrivate.m_curAudioDecoder = rdkPlaybackGroup.curAudioDecoder;
    playbackGroupPrivate.m_curAudioParse = rdkPlaybackGroup.curAudioParse;
    playbackGroupPrivate.m_curAudioTypefind = rdkPlaybackGroup.curAudioTypefind;
    playbackGroupPrivate.m_linkTypefindParser = rdkPlaybackGroup.linkTypefindParser;
    playbackGroupPrivate.m_isAudioAAC = rdkPlaybackGroup.isAudioAAC;
}

void convertRdkAudioAttributes(const rdk_gstreamer_utils::AudioAttributes &rdkAudioAttributes,
                               firebolt::rialto::server::AudioAttributesPrivate &audioAttributesPrivate)
{
    audioAttributesPrivate.m_codecParam = rdkAudioAttributes.mCodecParam;
    audioAttributesPrivate.m_numberOfChannels = rdkAudioAttributes.mNumberOfChannels;
    audioAttributesPrivate.m_samplesPerSecond = rdkAudioAttributes.mSamplesPerSecond;
    audioAttributesPrivate.m_bitrate = rdkAudioAttributes.mBitrate;
    audioAttributesPrivate.m_blockAlignment = rdkAudioAttributes.mBlockAlignment;
    audioAttributesPrivate.m_codecSpecificData = rdkAudioAttributes.mCodecSpecificData;
    audioAttributesPrivate.m_codecSpecificDataLen = rdkAudioAttributes.mCodecSpecificDataLen;
}
} // namespace

namespace firebolt::rialto::server
{
std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> IRdkGstreamerUtilsWrapperFactory::getFactory()
{
    static std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> factory;
    if (!factory)
    {
        factory = std::make_shared<RdkGstreamerUtilsWrapperFactory>();
    }
    return factory;
}

std::shared_ptr<IRdkGstreamerUtilsWrapper> RdkGstreamerUtilsWrapperFactory::createRdkGstreamerUtilsWrapper() const
{
    return std::make_shared<RdkGstreamerUtilsWrapper>();
}

bool RdkGstreamerUtilsWrapper::performAudioTrackCodecChannelSwitch(
    PlaybackGroupPrivate *playbackGroup, const void *sampleAttr, AudioAttributesPrivate *audioAttr,
    std::uint32_t *status, unsigned int *ui32Delay, long long *audioChangeTargetPts, const long long *currentDispPts,
    unsigned int *audioChangeStage, GstCaps **appsrcCaps, bool *audioaac, bool svpEnabled, GstElement *aSrc,
    bool *ret) const
{
    if (!playbackGroup || !audioAttr)
    {
        RIALTO_SERVER_LOG_ERROR("Playback group or audio attributes is NULL");
        return false;
    }
    auto rdkPlaybackGroup{convertPlaybackGroupPrivate(*playbackGroup)};
    auto rdkAudioAttributes{convertAudioAttributesPrivate(*audioAttr)};
    bool result{rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch(&rdkPlaybackGroup, sampleAttr,
                                                                         &rdkAudioAttributes, status, ui32Delay,
                                                                         audioChangeTargetPts, currentDispPts,
                                                                         audioChangeStage, appsrcCaps, audioaac,
                                                                         svpEnabled, aSrc, ret)};
    convertRdkPlaybackGroup(rdkPlaybackGroup, *playbackGroup);
    convertRdkAudioAttributes(rdkAudioAttributes, *audioAttr);
    return result;
}

void RdkGstreamerUtilsWrapper::deepElementAdded(PlaybackGroupPrivate *playbackGroup, GstBin *pipeline, GstBin *bin,
                                                GstElement *element) const
{
    if (!playbackGroup)
    {
        RIALTO_SERVER_LOG_ERROR("Playback group is NULL");
        return;
    }
    auto rdkPlaybackGroup{convertPlaybackGroupPrivate(*playbackGroup)};
    rdk_gstreamer_utils::deepElementAdded(&rdkPlaybackGroup, pipeline, bin, element);
    convertRdkPlaybackGroup(rdkPlaybackGroup, *playbackGroup);
}
} // namespace firebolt::rialto::server
