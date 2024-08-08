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

#include <string>
#include <cstdint>
#include <optional>
#include <memory>

#pragma once
class ITextTrackAccessor;
/**
 * @brief ITextTrackAccessorFactory factory class, for the ITextTrackAccessorFactory singleton object.
 */
class ITextTrackAccessorFactory
{
public:
    ITextTrackAccessorFactory() = default;
    virtual ~ITextTrackAccessorFactory() = default;

    /**
     * @brief Gets the ITextTrackAccessorFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<ITextTrackAccessorFactory> getFactory();

    /**
     * @brief Gets a ITextTrackAccessor singleton object.
     *
     * @retval the accessor instance or null on error.
     */
    virtual std::shared_ptr<ITextTrackAccessor> getTextTrackAccessor() = 0;
};

class ITextTrackAccessor
{
    public:
        enum class DataType
        {
            UNKNOWN,
            WebVTT,
            TTML
        };

        virtual ~ITextTrackAccessor() = default;

        virtual std::optional<uint32_t> openSession(const std::string &displayName) = 0;
        virtual bool closeSession(uint32_t sessionId) = 0;
        virtual bool pause(uint32_t sessionId) = 0;
        virtual bool play(uint32_t sessionId) = 0;
        virtual bool mute(uint32_t sessionId, bool mute) = 0;
        virtual bool setPosition(uint32_t sessionId, uint64_t mediaTimestampMs) = 0;
        virtual bool sendData(uint32_t sessionId, const std::string &data, DataType datatype,
                              int32_t displayOffsetMs = 0) = 0;
        virtual bool setSessionWebVTTSelection(uint32_t sessionId) = 0;
        virtual bool setSessionTTMLSelection(uint32_t sessionId) = 0;
};