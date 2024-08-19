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

#include "TextTrackSession.h"
#include "TextTrackAccessor.h"

TextTrackSession::TextTrackSession(const std::string &displayName,
                                   const std::shared_ptr<ITextTrackAccessorFactory> &textTrackAccessorFactory)
{
    if (!textTrackAccessorFactory)
    {
        RIALTO_SERVER_LOG_ERROR("Invalid TextTrackAccessorFactory");
        throw std::runtime_error("Invalid TextTrackAccessorFactory");
    }

    m_textTrackAccessor = textTrackAccessorFactory->getTextTrackAccessor();
    if (!m_textTrackAccessor)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get TextTrackAccessor");
        throw std::runtime_error("Failed to get TextTrackAccessor");
    }

    std::optional<uint32_t> sessionId = m_textTrackAccessor->openSession(displayName);
    if (!sessionId)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create TextTrack session");
        throw std::runtime_error("Failed to create TextTrack session");
    }

    m_sessionId = sessionId.value();
}

TextTrackSession::~TextTrackSession()
{
    m_textTrackAccessor->closeSession(m_sessionId);
}

bool TextTrackSession::pause()
{
    return m_textTrackAccessor->pause(m_sessionId);
}

bool TextTrackSession::play()
{
    return m_textTrackAccessor->play(m_sessionId);
}

bool TextTrackSession::mute(bool mute)
{
    return m_textTrackAccessor->mute(m_sessionId, mute);
}

bool TextTrackSession::setPosition(uint64_t mediaTimestampMs)
{
    return m_textTrackAccessor->setPosition(m_sessionId, mediaTimestampMs);
}

bool TextTrackSession::sendData(const std::string &data, int32_t displayOffsetMs)
{
    return m_textTrackAccessor->sendData(m_sessionId, data, m_dataType, displayOffsetMs);
}

bool TextTrackSession::setSessionWebVTTSelection()
{
    m_dataType = ITextTrackAccessor::DataType::WebVTT;
    return m_textTrackAccessor->setSessionWebVTTSelection(m_sessionId);
}

bool TextTrackSession::setSessionTTMLSelection()
{
    m_dataType = ITextTrackAccessor::DataType::TTML;
    return m_textTrackAccessor->setSessionTTMLSelection(m_sessionId);
}

bool TextTrackSession::setSessionCCSelection(const std::string &service)
{
    return m_textTrackAccessor->setSessionCCSelection(m_sessionId, service);
}