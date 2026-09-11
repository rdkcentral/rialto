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

#ifndef FIREBOLT_RIALTO_COMMON_DECODER_CAPABILITIES_UTILS_H_
#define FIREBOLT_RIALTO_COMMON_DECODER_CAPABILITIES_UTILS_H_

#include "AudioDecoderCapabilities.h"
#include "VideoDecoderCapabilities.h"

namespace firebolt::rialto::common
{
// Audio capability comparators
bool operator==(const AudioProfileCapability &lhs, const AudioProfileCapability &rhs);
bool operator==(const PcmCapability &lhs, const PcmCapability &rhs);
bool operator==(const AacCapability &lhs, const AacCapability &rhs);
bool operator==(const MpegAudioCapability &lhs, const MpegAudioCapability &rhs);
bool operator==(const Mp3Capability &lhs, const Mp3Capability &rhs);
bool operator==(const AlacCapability &lhs, const AlacCapability &rhs);
bool operator==(const SbcCapability &lhs, const SbcCapability &rhs);
bool operator==(const DolbyAc3Capability &lhs, const DolbyAc3Capability &rhs);
bool operator==(const DolbyAc4Capability &lhs, const DolbyAc4Capability &rhs);
bool operator==(const DolbyEac3Capability &lhs, const DolbyEac3Capability &rhs);
bool operator==(const DolbyTruehdCapability &lhs, const DolbyTruehdCapability &rhs);
bool operator==(const FlacCapability &lhs, const FlacCapability &rhs);
bool operator==(const VorbisCapability &lhs, const VorbisCapability &rhs);
bool operator==(const OpusCapability &lhs, const OpusCapability &rhs);
bool operator==(const RealAudioCapability &lhs, const RealAudioCapability &rhs);
bool operator==(const UsacCapability &lhs, const UsacCapability &rhs);
bool operator==(const DtsCapability &lhs, const DtsCapability &rhs);
bool operator==(const AvsCapability &lhs, const AvsCapability &rhs);
bool operator==(const AudioDecoderCapability &lhs, const AudioDecoderCapability &rhs);
bool operator==(const AudioDecoderCapabilities &lhs, const AudioDecoderCapabilities &rhs);

// Video capability comparators
bool operator==(const Mpeg2Profile &lhs, const Mpeg2Profile &rhs);
bool operator==(const H264Profile &lhs, const H264Profile &rhs);
bool operator==(const H265Profile &lhs, const H265Profile &rhs);
bool operator==(const Vp9Profile &lhs, const Vp9Profile &rhs);
bool operator==(const Av1Profile &lhs, const Av1Profile &rhs);
bool operator==(const Mpeg2CodecCapability &lhs, const Mpeg2CodecCapability &rhs);
bool operator==(const H264CodecCapability &lhs, const H264CodecCapability &rhs);
bool operator==(const H265CodecCapability &lhs, const H265CodecCapability &rhs);
bool operator==(const Vp9CodecCapability &lhs, const Vp9CodecCapability &rhs);
bool operator==(const Av1CodecCapability &lhs, const Av1CodecCapability &rhs);
bool operator==(const VideoCodecCapabilities &lhs, const VideoCodecCapabilities &rhs);
bool operator==(const VideoDecoderCapability &lhs, const VideoDecoderCapability &rhs);
bool operator==(const VideoDecoderCapabilities &lhs, const VideoDecoderCapabilities &rhs);
} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_DECODER_CAPABILITIES_UTILS_H_
