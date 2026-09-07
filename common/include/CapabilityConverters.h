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

#ifndef FIREBOLT_RIALTO_IPC_COMMON_CAPABILITY_CONVERTERS_H_
#define FIREBOLT_RIALTO_IPC_COMMON_CAPABILITY_CONVERTERS_H_

#include "mediaCapabilitiesCommon.pb.h"
#include <AudioDecoderCapabilities.h>
#include <VideoDecoderCapabilities.h>

// Single shared implementation of the AudioDecoderCapabilities/VideoDecoderCapabilities <-> typed proto
// AudioCapabilities/VideoCapabilities (mediaCapabilitiesCommon.proto) conversions. Used by ServerManager
// (serialise, for SetConfigurationRequest), RialtoServer (deserialise SetConfigurationRequest, and
// serialise for MediaCapabilitiesModule IPC responses) and RialtoClient (deserialise MediaCapabilitiesModule
// IPC responses) so the conversion logic only needs to be written once.
namespace firebolt::rialto::ipc::common
{
void serialiseAudioCapabilities(const firebolt::rialto::common::AudioDecoderCapabilities &src,
                                ::firebolt::rialto::AudioCapabilities *dst);
void serialiseVideoCapabilities(const firebolt::rialto::common::VideoDecoderCapabilities &src,
                                ::firebolt::rialto::VideoCapabilities *dst);

firebolt::rialto::common::AudioDecoderCapabilities
deserialiseAudioCapabilities(const ::firebolt::rialto::AudioCapabilities &src);
firebolt::rialto::common::VideoDecoderCapabilities
deserialiseVideoCapabilities(const ::firebolt::rialto::VideoCapabilities &src);
} // namespace firebolt::rialto::ipc::common

#endif // FIREBOLT_RIALTO_IPC_COMMON_CAPABILITY_CONVERTERS_H_
