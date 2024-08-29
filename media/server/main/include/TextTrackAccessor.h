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

#ifndef FIREBOLT_RIALTO_SERVER_TEXT_TRACK_ACCESSOR_H_
#define FIREBOLT_RIALTO_SERVER_TEXT_TRACK_ACCESSOR_H_

#include "ITextTrackPluginWrapper.h"
#include "ITextTrackWrapper.h"
#include "IThunderWrapper.h"
#include "RialtoServerLogging.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace firebolt::rialto::server
{
class TextTrackAccessor;

class TextTrackAccessorFactory
{
public:
    static TextTrackAccessorFactory &getFactory();
    std::shared_ptr<TextTrackAccessor> getTextTrackAccessor() const;
};

class TextTrackAccessor
{
public:
    enum class DataType
    {
        UNKNOWN,
        WebVTT,
        TTML
    };

    TextTrackAccessor(const std::shared_ptr<firebolt::rialto::wrappers::ITextTrackPluginWrapper> &textTrackPluginWrapper,
                      const std::shared_ptr<firebolt::rialto::wrappers::IThunderWrapper> &thunderWrapper);
    ~TextTrackAccessor();
    std::optional<uint32_t> openSession(const std::string &displayName);
    bool closeSession(uint32_t sessionId);
    bool pause(uint32_t sessionId);
    bool play(uint32_t sessionId);
    bool mute(uint32_t sessionId, bool mute);
    bool setPosition(uint32_t sessionId, uint64_t mediaTimestampMs);
    bool sendData(uint32_t sessionId, const std::string &data, DataType datatype, int32_t displayOffsetMs = 0);
    bool setSessionWebVTTSelection(uint32_t sessionId);
    bool setSessionTTMLSelection(uint32_t sessionId);
    bool setSessionCCSelection(uint32_t sessionId, const std::string &service);

private:
    bool createTextTrackControlInterface();

    std::shared_ptr<firebolt::rialto::wrappers::ITextTrackPluginWrapper> m_textTrackPluginWrapper;
    std::shared_ptr<firebolt::rialto::wrappers::IThunderWrapper> m_thunderWrapper;
    std::shared_ptr<firebolt::rialto::wrappers::ITextTrackWrapper> m_textTrackWrapper;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_TEXT_TRACK_ACCESSOR_H_
