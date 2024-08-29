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
#include "RialtoServerLogging.h"
#include "ITextTrackPluginWrapper.h"
#include "ITextTrackWrapper.h"
#include "IThunderWrapper.h"

/**
 * @brief ITextTrackAccessorFactory factory class definition.
 */
class TextTrackAccessorFactory : public ITextTrackAccessorFactory
{
public:
    /**
     * @brief Weak pointer to the singleton factroy object.
     */
    static std::weak_ptr<ITextTrackAccessorFactory> m_factory;

    /**
     * @brief Weak pointer to the singleton object.
     */
    static std::weak_ptr<ITextTrackAccessor> m_textTrackAccessor;

    std::shared_ptr<ITextTrackAccessor> getTextTrackAccessor() override;
};

class TextTrackAccessor : public ITextTrackAccessor
{
public:
    TextTrackAccessor(const std::shared_ptr<firebolt::rialto::wrappers::ITextTrackPluginWrapper> &textTrackPluginWrapper,
                      const std::shared_ptr<firebolt::rialto::wrappers::IThunderWrapper> &thunderWrapper);
    ~TextTrackAccessor();
    std::optional<uint32_t> openSession(const std::string &displayName) override;
    bool closeSession(uint32_t sessionId) override;
    bool pause(uint32_t sessionId) override;
    bool play(uint32_t sessionId) override;
    bool mute(uint32_t sessionId, bool mute) override;
    bool setPosition(uint32_t sessionId, uint64_t mediaTimestampMs) override;
    bool sendData(uint32_t sessionId, const std::string &data, DataType datatype, int32_t displayOffsetMs = 0) override;
    bool setSessionWebVTTSelection(uint32_t sessionId) override;
    bool setSessionTTMLSelection(uint32_t sessionId) override;
    bool setSessionCCSelection(uint32_t sessionId, const std::string &service) override;

private:
    bool createTextTrackControlInterface();

    std::shared_ptr<firebolt::rialto::wrappers::ITextTrackPluginWrapper> m_textTrackPluginWrapper;
    std::shared_ptr<firebolt::rialto::wrappers::IThunderWrapper> m_thunderWrapper;
    std::shared_ptr<firebolt::rialto::wrappers::ITextTrackWrapper> m_textTrackWrapper;
};
