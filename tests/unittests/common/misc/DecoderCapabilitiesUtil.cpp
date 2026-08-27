/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "DecoderCapabilitiesUtil.h"

namespace firebolt::rialto::common
{
bool operator==(const AudioProfileCapability &lhs, const AudioProfileCapability &rhs)
{
    return lhs.maxBitrateInBps == rhs.maxBitrateInBps && lhs.maxChannels == rhs.maxChannels &&
           lhs.maxSampleRateInHz == rhs.maxSampleRateInHz && lhs.maxBitDepth == rhs.maxBitDepth;
}

bool operator==(const PcmCapability &lhs, const PcmCapability &rhs)
{
    return lhs.base == rhs.base;
}

bool operator==(const AacCapability &lhs, const AacCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

bool operator==(const MpegAudioCapability &lhs, const MpegAudioCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

bool operator==(const Mp3Capability &lhs, const Mp3Capability &rhs)
{
    return lhs.base == rhs.base;
}

bool operator==(const AlacCapability &lhs, const AlacCapability &rhs)
{
    return lhs.base == rhs.base;
}

bool operator==(const SbcCapability &lhs, const SbcCapability &rhs)
{
    return lhs.base == rhs.base;
}

bool operator==(const DolbyAc3Capability &lhs, const DolbyAc3Capability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

bool operator==(const DolbyAc4Capability &lhs, const DolbyAc4Capability &rhs)
{
    return lhs.base == rhs.base;
}

bool operator==(const DolbyEac3Capability &lhs, const DolbyEac3Capability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

bool operator==(const DolbyTruehdCapability &lhs, const DolbyTruehdCapability &rhs)
{
    return lhs.base == rhs.base;
}

bool operator==(const FlacCapability &lhs, const FlacCapability &rhs)
{
    return lhs.base == rhs.base;
}

bool operator==(const VorbisCapability &lhs, const VorbisCapability &rhs)
{
    return lhs.base == rhs.base;
}

bool operator==(const OpusCapability &lhs, const OpusCapability &rhs)
{
    return lhs.base == rhs.base;
}

bool operator==(const RealAudioCapability &lhs, const RealAudioCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

bool operator==(const UsacCapability &lhs, const UsacCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

bool operator==(const DtsCapability &lhs, const DtsCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

bool operator==(const AvsCapability &lhs, const AvsCapability &rhs)
{
    return lhs.profiles == rhs.profiles;
}

bool operator==(const AudioDecoderCapability &lhs, const AudioDecoderCapability &rhs)
{
    return lhs.pcm == rhs.pcm && lhs.aac == rhs.aac && lhs.mpegAudio == rhs.mpegAudio && lhs.mp3 == rhs.mp3 &&
           lhs.alac == rhs.alac && lhs.sbc == rhs.sbc && lhs.dolbyAc3 == rhs.dolbyAc3 && lhs.dolbyAc4 == rhs.dolbyAc4 &&
           lhs.dolbyEac3 == rhs.dolbyEac3 && lhs.dolbyTruehd == rhs.dolbyTruehd && lhs.flac == rhs.flac &&
           lhs.vorbis == rhs.vorbis && lhs.opus == rhs.opus && lhs.realAudio == rhs.realAudio && lhs.usac == rhs.usac &&
           lhs.dts == rhs.dts && lhs.avs == rhs.avs;
}

bool operator==(const AudioDecoderCapabilities &lhs, const AudioDecoderCapabilities &rhs)
{
    return lhs.interfaceVersion == rhs.interfaceVersion && lhs.schemaVersion == rhs.schemaVersion &&
           lhs.capabilities == rhs.capabilities;
}

bool operator==(const Mpeg2Profile &lhs, const Mpeg2Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

bool operator==(const H264Profile &lhs, const H264Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

bool operator==(const H265Profile &lhs, const H265Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

bool operator==(const Vp9Profile &lhs, const Vp9Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

bool operator==(const Av1Profile &lhs, const Av1Profile &rhs)
{
    return lhs.type == rhs.type && lhs.maxLevel == rhs.maxLevel && lhs.maxBitrateInBps == rhs.maxBitrateInBps;
}

bool operator==(const Mpeg2CodecCapability &lhs, const Mpeg2CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

bool operator==(const H264CodecCapability &lhs, const H264CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

bool operator==(const H265CodecCapability &lhs, const H265CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

bool operator==(const Vp9CodecCapability &lhs, const Vp9CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

bool operator==(const Av1CodecCapability &lhs, const Av1CodecCapability &rhs)
{
    return lhs.profiles == rhs.profiles && lhs.dynamicRanges == rhs.dynamicRanges;
}

bool operator==(const VideoCodecCapabilities &lhs, const VideoCodecCapabilities &rhs)
{
    return lhs.mpeg2 == rhs.mpeg2 && lhs.h264 == rhs.h264 && lhs.h265 == rhs.h265 && lhs.vp9 == rhs.vp9 &&
           lhs.av1 == rhs.av1;
}

bool operator==(const VideoDecoderCapability &lhs, const VideoDecoderCapability &rhs)
{
    return lhs.codecCapabilities == rhs.codecCapabilities;
}

bool operator==(const VideoDecoderCapabilities &lhs, const VideoDecoderCapabilities &rhs)
{
    return lhs.interfaceVersion == rhs.interfaceVersion && lhs.schemaVersion == rhs.schemaVersion &&
           lhs.capabilities == rhs.capabilities;
}
} // namespace firebolt::rialto::common
