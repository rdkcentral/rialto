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

#pragma once

#include "ITextTrackAccessor.h"
#include <memory>

class TextTrackSession
{
public:
    TextTrackSession(const std::string &displayName,
                     const std::shared_ptr<ITextTrackAccessorFactory> &textTrackAccessorFactory);
    ~TextTrackSession();
    bool pause();
    bool play();
    bool mute(bool mute);
    bool setPosition(uint64_t mediaTimestampMs);
    bool sendData(const std::string &data, int32_t displayOffsetMs = 0);
    bool setSessionWebVTTSelection();
    bool setSessionTTMLSelection();

private:
    std::shared_ptr<ITextTrackAccessor> m_textTrackAccessor;
    ITextTrackAccessor::DataType m_dataType{ITextTrackAccessor::DataType::UNKNOWN};
    uint32_t m_sessionId{0};
};
