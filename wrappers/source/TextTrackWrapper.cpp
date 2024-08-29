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

#include "TextTrackWrapper.h"

namespace
{
WPEFramework::Exchange::ITextTrack::DataType convertDataType(const firebolt::rialto::wrappers::ITextTrackWrapper::DataType &dataType)
{
    switch (dataType)
    {
    case firebolt::rialto::wrappers::ITextTrackWrapper::DataType::PES:
        return WPEFramework::Exchange::ITextTrack::DataType::PES;
    case firebolt::rialto::wrappers::ITextTrackWrapper::DataType::TTML:
        return WPEFramework::Exchange::ITextTrack::DataType::TTML;
    case firebolt::rialto::wrappers::ITextTrackWrapper::DataType::CC:
        return WPEFramework::Exchange::ITextTrack::DataType::CC;
    case firebolt::rialto::wrappers::ITextTrackWrapper::DataType::WEBVTT:
        return WPEFramework::Exchange::ITextTrack::DataType::WEBVTT;
    }
    return WPEFramework::Exchange::ITextTrack::DataType::PES;
}
} // namespace

namespace firebolt::rialto::wrappers
{
TextTrackWrapper::TextTrackWrapper(WPEFramework::Exchange::ITextTrack *textTrackControlInterface)
: m_textTrackControlInterface{textTrackControlInterface}
{
}

TextTrackWrapper::~TextTrackWrapper()
{
    if (m_textTrackControlInterface)
    {
        m_textTrackControlInterface->Release();
    }
}

std::uint32_t TextTrackWrapper::openSession(const std::string &displayName, std::uint32_t &sessionId) const
{
    return m_textTrackControlInterface->OpenSession(displayName, sessionId);
}

std::uint32_t TextTrackWrapper::closeSession(std::uint32_t sessionId) const
{
    return m_textTrackControlInterface->CloseSession(sessionId);
}

std::uint32_t TextTrackWrapper::pauseSession(std::uint32_t sessionId) const
{
    return m_textTrackControlInterface->PauseSession(sessionId);
}

std::uint32_t TextTrackWrapper::resumeSession(std::uint32_t sessionId) const
{
    return m_textTrackControlInterface->ResumeSession(sessionId);
}

std::uint32_t TextTrackWrapper::muteSession(std::uint32_t sessionId) const
{
    return m_textTrackControlInterface->MuteSession(sessionId);
}

std::uint32_t TextTrackWrapper::unmuteSession(std::uint32_t sessionId) const
{
    return m_textTrackControlInterface->UnMuteSession(sessionId);
}

std::uint32_t TextTrackWrapper::sendSessionTimestamp(std::uint32_t sessionId, std::uint64_t mediaTimestampMs) const
{
    return m_textTrackControlInterface->SendSessionTimestamp(sessionId, mediaTimestampMs);
}

std::uint32_t TextTrackWrapper::sendSessionData(std::uint32_t sessionId, ITextTrackWrapper::DataType type, std::int32_t displayOffsetMs, const std::string &data) const
{
    return m_textTrackControlInterface->SendSessionData(sessionId, convertDataType(type), displayOffsetMs, data);
}

std::uint32_t TextTrackWrapper::setSessionWebVTTSelection(std::uint32_t sessionId) const
{
    return m_textTrackControlInterface->SetSessionWebVTTSelection(sessionId);
}

std::uint32_t TextTrackWrapper::setSessionTTMLSelection(std::uint32_t sessionId) const
{
    return m_textTrackControlInterface->SetSessionTTMLSelection(sessionId);
}

std::uint32_t TextTrackWrapper::setSessionClosedCaptionsService(std::uint32_t sessionId, const std::string &service) const
{
    return m_textTrackControlInterface->SetSessionClosedCaptionsService(sessionId, service);
}
} // namespace firebolt::rialto::wrappers
