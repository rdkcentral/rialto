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

#ifndef FIREBOLT_RIALTO_COMMON_DECODER_CAPABILITIES_COMMON_H_
#define FIREBOLT_RIALTO_COMMON_DECODER_CAPABILITIES_COMMON_H_

/**
 * @file DecoderCapabilitiesCommon.h
 *
 * Common types and enums for decoder capabilities
 *
 */

namespace firebolt::rialto::common
{
/**
 * @brief Status of the decoder capabilities config file read operation.
 */
enum class DecoderCapabilitiesStatus
{
    OK,
    CONFIG_NOT_FOUND,         /**< Config file not found */
    SCHEMA_VALIDATION_FAILED, /**< Config file failed schema validation */
    INTERNAL_ERROR
};

} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_DECODER_CAPABILITIES_COMMON_H_
