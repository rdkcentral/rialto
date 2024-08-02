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

#include <iostream>
#include <cinttypes>

using namespace WPEFramework;

//

//
TextTrackAccessor::TextTrackAccessor(const std::string &displayName)
{
    if (!textTrackControlInterface())
    {
        throw std::runtime_error("Failed to create TextTrack interfaces");
    }

    uint32_t result = 888;
    result = m_textTrackControlInterface->OpenSession(displayName, m_sessionId);
    RIALTO_SERVER_LOG_ERROR("KLOPS sessionId %u, sessionId2 %u, displayName %s", m_sessionId, result, displayName.c_str());
    std::string webvttContent = 
    "WEBVTT\n\n"
    "00:00:00.000 --> 00:00:00.500\n"
    "DUUUUUUUUUUUUUUPAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n\n"
    "00:00:00.500 --> 00:00:01.000\n"
    "BLAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n\n"
    "00:00:01.000 --> 00:00:01.500\n"
    "TAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANCCCCCCCCCCCCCCCCCCCCOOOOOOOOWAAAAALA\n\n"
    "00:00:01.500 --> 00:00:02.000\n"
    "DUUUUUUUUUUUUUUPAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n\n"
    "00:00:02.000 --> 00:00:02.500\n"
    "STUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHI\n\n"
    "00:00:02.500 --> 00:00:03.000\n"
    "JKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ\n\n"
    "00:00:03.000 --> 00:00:03.500\n"
    "DUUUUUUUUUUUUUUPAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n\n"
    "00:00:03.500 --> 00:00:04.000\n"
    "STUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHI\n\n"
    "00:00:04.000 --> 00:00:04.500\n"
    "JKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ\n\n"
    "00:00:04.500 --> 00:00:05.000\n"
    "DUUUUUUUUUUUUUUPAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";

    // Exchange::ITextTrackClosedCaptionsStyle::ClosedCaptionsStyle style;
    // style.fontColor = "#FF0000";
    // style.fontOpacity = 100;
    // style.backgroundColor = "#000000";
    // style.windowColor = "#FFFF00";
    // style.windowOpacity = 100;
    // style.fontSize = Exchange::ITextTrackClosedCaptionsStyle::FontSize::EXTRA_LARGE;
    
     RIALTO_SERVER_LOG_ERROR("KLOPS SetSessionWebVTTSelection %u",m_textTrackControlInterface->SetSessionWebVTTSelection(m_sessionId));
   // RIALTO_SERVER_LOG_ERROR("KLOPS SetSessionWebVTTSelection %u",m_textTrackControlInterface->ApplyCustomClosedCaptionsStyleToSession(m_sessionId, style));
    // RIALTO_SERVER_LOG_ERROR("KLOPS SetSessionWebVTTSelection %u",m_textTrackControlInterface->SetSessionWebVTTSelection(m_sessionId));
    RIALTO_SERVER_LOG_ERROR("KLOPS Send session data %u", m_textTrackControlInterface->SendSessionData(m_sessionId, Exchange::ITextTrack::DataType::WEBVTT, 0, webvttContent));
    RIALTO_SERVER_LOG_ERROR("KLOPS settimestamp %u", m_textTrackControlInterface->SendSessionTimestamp(m_sessionId, 0));
    // RIALTO_SERVER_LOG_ERROR("KLOPS unmute %u",m_textTrackControlInterface->UnMuteSession(m_sessionId));
   //  RIALTO_SERVER_LOG_ERROR("KLOPS resume %u",m_textTrackControlInterface->ResumeSession(m_sessionId));
}

TextTrackAccessor::~TextTrackAccessor()
{
    if (m_textTrackControlInterface)
    {
        m_textTrackControlInterface->Release();
    }

    m_textTrackPlugin.Close(RPC::CommunicationTimeOut);
}

    bool TextTrackAccessor::pause()
    {
        uint32_t result = m_textTrackControlInterface->PauseSession(m_sessionId);
        if(result == Core::ERROR_NONE)
        {
            RIALTO_SERVER_LOG_INFO("TextTrack session %u paused", m_sessionId);
            return true;
        }

        RIALTO_SERVER_LOG_ERROR("Failed to pause TextTrack session %u; error %s", m_sessionId,
                                Core::ErrorToString(result));
        return false;
    }

    bool TextTrackAccessor::play()
    {
        uint32_t result = m_textTrackControlInterface->ResumeSession(m_sessionId);
        if(result == WPEFramework::Core::ERROR_NONE)
        {
            RIALTO_SERVER_LOG_INFO("TextTrack session %u resumed", m_sessionId);
            return true;
        }

        RIALTO_SERVER_LOG_ERROR("Failed to resume TextTrack session %u; error %s", m_sessionId,
                                Core::ErrorToString(result));
        return false;
    }

    bool TextTrackAccessor::mute(bool mute)
    {
        if (mute)
        {
            uint32_t result = m_textTrackControlInterface->MuteSession(m_sessionId);
            if (result == WPEFramework::Core::ERROR_NONE)
            {
                RIALTO_SERVER_LOG_INFO("TextTrack session %u muted", m_sessionId);
                return true;
            }

            RIALTO_SERVER_LOG_ERROR("Failed to mute TextTrack session %u; error %s", m_sessionId,
                                    Core::ErrorToString(result));
        }
        else
        {
            uint32_t result = m_textTrackControlInterface->UnMuteSession(m_sessionId);
            if (result == WPEFramework::Core::ERROR_NONE)
            {
                RIALTO_SERVER_LOG_INFO("TextTrack session %u unmuted", m_sessionId);
                return true;
            }

            RIALTO_SERVER_LOG_ERROR("Failed to unmute TextTrack session %u; error %s", m_sessionId,
                                    Core::ErrorToString(result));
        }

        return false;
    }

    bool TextTrackAccessor::setPosition(uint64_t mediaTimestampMs)
    {
        uint32_t result = m_textTrackControlInterface->SendSessionTimestamp(m_sessionId, mediaTimestampMs);
        if (result == WPEFramework::Core::ERROR_NONE)
        {
            RIALTO_SERVER_LOG_INFO("TextTrack session %u set position to %" PRIu64, m_sessionId, mediaTimestampMs);
            return true;
        }

        RIALTO_SERVER_LOG_ERROR("Failed to set position of TextTrack session %u to  %" PRIu64 "; error %s", m_sessionId,
                                mediaTimestampMs, Core::ErrorToString(result));
        return false;
    }

    bool TextTrackAccessor::sendData(const std::string &data, int32_t displayOffsetMs)
    {
        Exchange::ITextTrack::DataType dataType = Exchange::ITextTrack::DataType::WEBVTT; //todo-klops: unhardcode
        const uint32_t result =
            m_textTrackControlInterface->SendSessionData(m_sessionId, dataType, displayOffsetMs, data);
        if (result == WPEFramework::Core::ERROR_NONE)
        {
            RIALTO_SERVER_LOG_DEBUG("Sending data to TextTrack session %u was successful", m_sessionId);
            return true;
        }

        RIALTO_SERVER_LOG_ERROR("Failed to send data to TextTrack session %u; error %s", m_sessionId,
                                Core::ErrorToString(result));
        return false;
    }

bool TextTrackAccessor::textTrackControlInterface()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if(m_textTrackPlugin.IsOperational() && m_textTrackControlInterface)
    {
        RIALTO_SERVER_LOG_DEBUG("TextTrack interface already created");
        return true;
    }

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

bool TextTrackAccessor::setSessionWebVTTSelection()
{
    uint32_t result = m_textTrackControlInterface->SetSessionWebVTTSelection(m_sessionId);
    if (result == WPEFramework::Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_DEBUG("Setting WebVTT selection for session %u was successful", m_sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to set WebVTT selection for session %u; error %s", m_sessionId,
                            Core::ErrorToString(result));
    return false;
}

bool TextTrackAccessor::setSessionTTMLSelection()
{
    uint32_t result = m_textTrackControlInterface->SetSessionTTMLSelection(m_sessionId);
    if (result == WPEFramework::Core::ERROR_NONE)
    {
        RIALTO_SERVER_LOG_DEBUG("Setting TTML selection for session %u was successful", m_sessionId);
        return true;
    }

    RIALTO_SERVER_LOG_ERROR("Failed to set TTML selection for session %u; error %s", m_sessionId,
                            Core::ErrorToString(result));
    return false;
}

TextTrackAccessor &TextTrackAccessor::instance()
{
    static TextTrackAccessor accessor;
    return accessor;
}
