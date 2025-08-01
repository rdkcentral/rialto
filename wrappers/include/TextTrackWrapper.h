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

#ifndef FIREBOLT_RIALTO_WRAPPERS_TEXT_TRACK_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_TEXT_TRACK_WRAPPER_H_

#ifdef RIALTO_ENABLE_TEXT_TRACK

#include "ITextTrackWrapper.h"
#include <interfaces/ITextTrack.h>
#include <string>

namespace firebolt::rialto::wrappers
{
class TextTrackWrapper : public ITextTrackWrapper
{
public:
    explicit TextTrackWrapper(WPEFramework::Exchange::ITextTrack *textTrackControlInterface);
    ~TextTrackWrapper() override;

    std::uint32_t openSession(const std::string &displayName, std::uint32_t &sessionId) const override;
    std::uint32_t closeSession(std::uint32_t sessionId) const override;
    std::uint32_t pauseSession(std::uint32_t sessionId) const override;
    std::uint32_t resumeSession(std::uint32_t sessionId) const override;
    std::uint32_t muteSession(std::uint32_t sessionId) const override;
    std::uint32_t unmuteSession(std::uint32_t sessionId) const override;
    std::uint32_t resetSession(std::uint32_t sessionId) const override;
    std::uint32_t sendSessionTimestamp(std::uint32_t sessionId, std::uint64_t mediaTimestampMs) const override;
    std::uint32_t sendSessionData(std::uint32_t sessionId, ITextTrackWrapper::DataType type,
                                  std::int64_t displayOffsetMs, const std::string &data) const override;
    std::uint32_t setSessionWebVTTSelection(std::uint32_t sessionId) const override;
    std::uint32_t setSessionTTMLSelection(std::uint32_t sessionId) const override;
    std::uint32_t setSessionClosedCaptionsService(std::uint32_t sessionId, const std::string &service) const override;

private:
    WPEFramework::Exchange::ITextTrack *m_textTrackControlInterface;
};
} // namespace firebolt::rialto::wrappers

#endif // RIALTO_ENABLE_TEXT_TRACK

#endif // FIREBOLT_RIALTO_WRAPPERS_TEXT_TRACK_WRAPPER_H_
