/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef FIREBOLT_RIALTO_CLIENT_MEDIA_CAPABILITIES_IPC_CONVERTERS_H_
#define FIREBOLT_RIALTO_CLIENT_MEDIA_CAPABILITIES_IPC_CONVERTERS_H_

#include "AudioDecoderCapabilities.h"
#include "RialtoCommonIpc.h"
#include "VideoDecoderCapabilities.h"
#include "mediapipelinecapabilitiesmodule.pb.h"

namespace firebolt::rialto::client
{
/**
 * @brief Convert protobuf GetSupportedAudioCapabilitiesResponse to common::AudioDecoderCapabilities
 *
 * @param[in] response : The protobuf response to convert
 * @return The converted AudioDecoderCapabilities
 */
firebolt::rialto::common::AudioDecoderCapabilities
convertAudioDecoderCapabilities(const firebolt::rialto::GetSupportedAudioCapabilitiesResponse &response);

/**
 * @brief Convert protobuf GetSupportedVideoCapabilitiesResponse to common::VideoDecoderCapabilities
 *
 * @param[in] response : The protobuf response to convert
 * @return The converted VideoDecoderCapabilities
 */
firebolt::rialto::common::VideoDecoderCapabilities
convertVideoDecoderCapabilities(const firebolt::rialto::GetSupportedVideoCapabilitiesResponse &response);

} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_MEDIA_CAPABILITIES_IPC_CONVERTERS_H_
