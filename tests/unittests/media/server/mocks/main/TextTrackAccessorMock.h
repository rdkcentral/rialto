/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_TEXT_TRACK_ACCESSOR_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_TEXT_TRACK_ACCESSOR_MOCK_H_

#include "ITextTrackAccessor.h"
#include <gmock/gmock.h>
#include <string>

namespace firebolt::rialto::server
{
class TextTrackAccessorMock : public ITextTrackAccessor
{
public:
    MOCK_METHOD(std::optional<uint32_t>, openSession, (const std::string &displayName), (override));
    MOCK_METHOD(bool, closeSession, (uint32_t sessionId), (override));
    MOCK_METHOD(bool, resetSession, (uint32_t sessionId), (override));
    MOCK_METHOD(bool, pause, (uint32_t sessionId), (override));
    MOCK_METHOD(bool, play, (uint32_t sessionId), (override));
    MOCK_METHOD(bool, mute, (uint32_t sessionId, bool mute), (override));
    MOCK_METHOD(bool, setPosition, (uint32_t sessionId, uint64_t mediaTimestampMs), (override));
    MOCK_METHOD(bool, sendData,
                (uint32_t sessionId, const std::string &data, DataType datatype, int32_t displayOffsetMs), (override));
    MOCK_METHOD(bool, setSessionWebVTTSelection, (uint32_t sessionId), (override));
    MOCK_METHOD(bool, setSessionTTMLSelection, (uint32_t sessionId), (override));
    MOCK_METHOD(bool, setSessionCCSelection, (uint32_t sessionId, const std::string &service), (override));
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_TEXT_TRACK_ACCESSOR_MOCK_H_
