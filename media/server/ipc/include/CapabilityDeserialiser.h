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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_CAPABILITY_DESERIALISER_H_
#define FIREBOLT_RIALTO_SERVER_IPC_CAPABILITY_DESERIALISER_H_

#include "servermanagermodule.pb.h"
#include <AudioDecoderCapabilities.h>
#include <VideoDecoderCapabilities.h>

// Inverse of rialto::servermanager::ipc::serialiseAudioCapabilities/serialiseVideoCapabilities
// (CapabilitySerialiser.h in serverManager/ipc) - deserialises the capabilities forwarded by
// ServerManager in SetConfigurationRequest back into the full C++ structs on the RialtoServer side.
namespace firebolt::rialto::server::ipc
{
firebolt::rialto::common::AudioDecoderCapabilities
deserialiseAudioCapabilities(const ::firebolt::rialto::AudioCapabilities &src);
firebolt::rialto::common::VideoDecoderCapabilities
deserialiseVideoCapabilities(const ::firebolt::rialto::VideoCapabilities &src);
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_CAPABILITY_DESERIALISER_H_
