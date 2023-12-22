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

#ifndef FIREBOLT_RIALTO_CLIENT_CT_COMMON_CONSTANTS_H_
#define FIREBOLT_RIALTO_CLIENT_CT_COMMON_CONSTANTS_H_

#include <utility>
#include <vector>

#include "MediaCommon.h"

namespace firebolt::rialto::client::ct
{
constexpr int32_t kKeySessionId{999};
const std::vector<std::uint8_t> kInitData{0x4C, 0x69, 0x63, 0x65, 0x6E, 0x73, 0x65, 0x20,
                                          0x4B, 0x65, 0x79, 0x20, 0x31, 0x32, 0x33, 0x34};
const KeyStatusVector
    kKeyStatuses{std::make_pair(std::vector<unsigned char>{'q', '3', 'p'}, firebolt::rialto::KeyStatus::USABLE),
                 std::make_pair(std::vector<unsigned char>{'f', '6', 'a'}, firebolt::rialto::KeyStatus::USABLE),
                 std::make_pair(std::vector<unsigned char>{'l', 'q', '1'}, firebolt::rialto::KeyStatus::USABLE),
                 std::make_pair(std::vector<unsigned char>{'p', 'r', '3'}, firebolt::rialto::KeyStatus::EXPIRED),
                 std::make_pair(std::vector<unsigned char>{'h', ':', 'd'},
                                firebolt::rialto::KeyStatus::OUTPUT_RESTRICTED)};
} // namespace firebolt::rialto::client::ct

#endif // FIREBOLT_RIALTO_CLIENT_CT_COMMON_CONSTANTS_H_
