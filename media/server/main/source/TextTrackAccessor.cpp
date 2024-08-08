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

#include "TextTrackAccessor.h"

#include <cinttypes>
#include <iostream>

//todo-klops namespace
using namespace WPEFramework;

std::weak_ptr<ITextTrackAccessorFactory> TextTrackAccessorFactory::m_factory;

std::shared_ptr<ITextTrackAccessorFactory> ITextTrackAccessorFactory::getFactory()
{
        std::shared_ptr<ITextTrackAccessorFactory> factory = TextTrackAccessorFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<TextTrackAccessorFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the TextTrackAccessor factory, reason: %s", e.what());
        }

        TextTrackAccessorFactory::m_factory = factory;
    }

    return factory;
}

std::shared_ptr<ITextTrackAccessor> TextTrackAccessorFactory::getTextTrackAccessor()
{
    static std::shared_ptr<ITextTrackAccessor> textTrackAccessor{};
    if (!textTrackAccessor)
    {
        try
        {
            textTrackAccessor = std::make_shared<TextTrackAccessor>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the TextTrackAccessor, reason: %s", e.what());
        }
    }

    return textTrackAccessor;
}

TextTrackAccessor::TextTrackAccessor()
{
    if (!textTrackControlInterface())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create TextTrack interfaces");
        throw std::runtime_error("Failed to create TextTrack interfaces");
    }
}

TextTrackAccessor::~TextTrackAccessor()
{
    if (m_textTrackControlInterface)
    {
        //m_textTrackControlInterface->CloseSession(sessionId);
        m_textTrackControlInterface->Release();
    }

    m_textTrackPlugin.Close(RPC::CommunicationTimeOut);
}

std::optional<uint32_t> TextTrackAccessor::openSession(const std::string &displayName)
{
    uint32_t sessionId = {};
    uint32_t result = m_textTrackControlInterface->OpenSession(displayName, sessionId);
    if (result == Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_INFO("TextTrack session %u created with display '%s'", sessionId, displayName.c_str());
        return sessionId;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to create TextTrack session with display '%s'; error '%s'", displayName.c_str(),
                            Core::ErrorToString(result));
    return std::nullopt;
}

bool TextTrackAccessor::closeSession(uint32_t sessionId)
{
    uint32_t result = m_textTrackControlInterface->CloseSession(sessionId);
    if (result == Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_INFO("TextTrack session %u closed", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to close TextTrack session %u; error %s", sessionId, Core::ErrorToString(result));
    return false;
}

bool TextTrackAccessor::pause(uint32_t sessionId)
{
    uint32_t result = m_textTrackControlInterface->PauseSession(sessionId);
    if (result == Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_INFO("TextTrack session %u paused", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to pause TextTrack session %u; error %s", sessionId, Core::ErrorToString(result));
    return false;
}

bool TextTrackAccessor::play(uint32_t sessionId)
{
    uint32_t result = m_textTrackControlInterface->ResumeSession(sessionId);
    if (result == WPEFramework::Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_INFO("TextTrack session %u resumed", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to resume TextTrack session %u; error %s", sessionId, Core::ErrorToString(result));
    return false;
}

bool TextTrackAccessor::mute(uint32_t sessionId, bool mute)
{
    if (mute)
    {
        uint32_t result = m_textTrackControlInterface->MuteSession(sessionId);
        if (result == WPEFramework::Core::ERROR_NONE)
        {
            RIALTO_SERVER_LOG_INFO("TextTrack session %u muted", sessionId);
            return true;
        }

        RIALTO_SERVER_LOG_ERROR("Failed to mute TextTrack session %u; error %s", sessionId,
                                Core::ErrorToString(result));
    }
    else
    {
        uint32_t result = m_textTrackControlInterface->UnMuteSession(sessionId);
        if (result == WPEFramework::Core::ERROR_NONE)
        {
            RIALTO_SERVER_LOG_INFO("TextTrack session %u unmuted", sessionId);
            return true;
        }

        RIALTO_SERVER_LOG_ERROR("Failed to unmute TextTrack session %u; error %s", sessionId,
                                Core::ErrorToString(result));
    }

    return false;
}

bool TextTrackAccessor::setPosition(uint32_t sessionId, uint64_t mediaTimestampMs)
{
    uint32_t result = m_textTrackControlInterface->SendSessionTimestamp(sessionId, mediaTimestampMs);
    if (result == WPEFramework::Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_INFO("TextTrack session %u set position to %" PRIu64, sessionId, mediaTimestampMs);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to set position of TextTrack session %u to  %" PRIu64 "; error %s", sessionId,
                            mediaTimestampMs, Core::ErrorToString(result));
    return false;
}

bool TextTrackAccessor::sendData(uint32_t sessionId, const std::string &data, DataType datatype, int32_t displayOffsetMs)
{
    Exchange::ITextTrack::DataType wpeDataType{};
    if (datatype == DataType::WebVTT)
    {
        wpeDataType = WPEFramework::Exchange::ITextTrack::DataType::TTML;
    }
    else if (datatype == DataType::TTML)
    {
        wpeDataType = WPEFramework::Exchange::ITextTrack::DataType::WEBVTT;
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Unknown data type");
        return false;
    }

    const uint32_t result = m_textTrackControlInterface->SendSessionData(sessionId, wpeDataType, displayOffsetMs, data);
    if (result == WPEFramework::Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_DEBUG("KLOPS Sending data to TextTrack session %u was successful", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("KLOPS Failed to send data to TextTrack session %u; error %s", sessionId,
                            Core::ErrorToString(result));
    return false;
}

bool TextTrackAccessor::textTrackControlInterface()
{
    uint32_t openResult = m_textTrackPlugin.Open(WPEFramework::RPC::CommunicationTimeOut, m_textTrackPlugin.Connector(),
                                                 "org.rdk.TextTrack");
    if (openResult == WPEFramework::Core::ERROR_NONE)
    {
        if (m_textTrackPlugin.IsOperational())
        {
            m_textTrackControlInterface = m_textTrackPlugin.Interface();
            if (m_textTrackControlInterface)
            {
                RIALTO_SERVER_LOG_INFO("Created TextTrack interface");
                return true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to create TextTrack interface");
            }
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("TextTrack plugin is NOT operational");
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to open TextTrack plugin; error '%s'",
                                WPEFramework::Core::ErrorToString(openResult));
    }

    return false;
}

bool TextTrackAccessor::setSessionWebVTTSelection(uint32_t sessionId)
{
    uint32_t result = m_textTrackControlInterface->SetSessionWebVTTSelection(sessionId);
    //todo-klops
    m_textTrackControlInterface->SendSessionTimestamp(sessionId, 0);
    if (result == WPEFramework::Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_DEBUG("Setting WebVTT selection for session %u was successful", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to set WebVTT selection for session %u; error %s", sessionId,
                            Core::ErrorToString(result));
    return false;
}

bool TextTrackAccessor::setSessionTTMLSelection(uint32_t sessionId)
{
    uint32_t result = m_textTrackControlInterface->SetSessionTTMLSelection(sessionId);
    if (result == WPEFramework::Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_DEBUG("Setting TTML selection for session %u was successful", sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to set TTML selection for session %u; error %s", sessionId,
                            Core::ErrorToString(result));
    return false;
}
