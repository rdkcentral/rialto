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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_TEXT_TRACK_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_TEXT_TRACK_WRAPPER_H_

#include <cstdint>
#include <string>

namespace firebolt::rialto::wrappers
{
class ITextTrackWrapper
{
public:
    enum class DataType : uint8_t
    {
        PES = 0,
        TTML = 1,
        CC = 2,
        WEBVTT = 3
    };

    ITextTrackWrapper() = default;
    virtual ~ITextTrackWrapper() = default;

    ITextTrackWrapper(const ITextTrackWrapper &) = delete;
    ITextTrackWrapper &operator=(const ITextTrackWrapper &) = delete;
    ITextTrackWrapper(ITextTrackWrapper &&) = delete;
    ITextTrackWrapper &operator=(ITextTrackWrapper &&) = delete;

    /**
     * @brief Opens a new renderSession.
     *
     * @param[in]  displayHandle : displayHandle is an encoding of the wayland display name optionally including the and
     * window ID
     * @param[out] sessionId     : On success the returned session id
     *
     * @retval the error code
     */
    virtual std::uint32_t openSession(const std::string &displayName, std::uint32_t &sessionId) const = 0;

    /**
     * @brief Closes a previously opened render session.
     *
     * @param[in] sessionId : the session to close
     *
     * @retval the error code
     */
    virtual std::uint32_t closeSession(std::uint32_t sessionId) const = 0;

    /**
     * @brief Pauses a render session.
     *
     * @param[in] sessionId : the session id
     *
     * @retval the error code
     */
    virtual std::uint32_t pauseSession(std::uint32_t sessionId) const = 0;

    /**
     * @brief Resumes a paused session
     *
     * @param[in] sessionId : the session id
     *
     * @retval the error code
     */
    virtual std::uint32_t resumeSession(std::uint32_t sessionId) const = 0;

    /**
     * @brief Mute will hide rendering of Captions
     *
     * @param[in] sessionId : the session id
     *
     * @retval the error code
     */
    virtual std::uint32_t muteSession(std::uint32_t sessionId) const = 0;

    /**
     * @brief UnMute will unhide the rendering of Captions.
     *
     * @param[in] sessionId : the session id
     *
     * @retval the error code
     */
    virtual std::uint32_t unmuteSession(std::uint32_t sessionId) const = 0;

    /**
     * @brief Resets the render session.
     *
     * @param[in] sessionId : the session id
     *
     * @retval the error code
     */
    virtual std::uint32_t resetSession(std::uint32_t sessionId) const = 0;

    /**
     * @brief Sends the current timestamp from a media player to a render session.
     *
     * @param[in] sessionId        : the session id
     * @param[in] mediaTimestampMs : the timestamp
     *
     * @retval the error code
     */
    virtual std::uint32_t sendSessionTimestamp(std::uint32_t sessionId, std::uint64_t mediaTimestampMs) const = 0;

    /**
     * @brief Sends data of Closed Captions, Captions or Timed Text data to a render session.
     *
     * @param[in] sessionId       : the session id
     * @param[in] type            : the type of data
     * @param[in] displayOffsetMs : currently unused
     * @param[in] data            : the data to display, properly formatted as per the expectations of the type used
     *
     * @retval the error code
     */
    virtual std::uint32_t sendSessionData(std::uint32_t sessionId, DataType type, std::int32_t displayOffsetMs,
                                          const std::string &data) const = 0;

    /**
     * @brief Set the render session into WebVTT mode
     *
     * @param[in] sessionId : the session id
     *
     * @retval the error code
     */
    virtual std::uint32_t setSessionWebVTTSelection(std::uint32_t sessionId) const = 0;

    /**
     * @brief Set the render session into TTML mode
     *
     * @param[in] sessionId : the session id
     *
     * @retval the error code
     */
    virtual std::uint32_t setSessionTTMLSelection(std::uint32_t sessionId) const = 0;

    /**
     * @brief Sets the render session into CC mode.
     *
     * @param[in] sessionId : the session id
     * @param[in] service   : the service to display e.g. "CC3"
     *
     * @retval the error code
     */
    virtual std::uint32_t setSessionClosedCaptionsService(std::uint32_t sessionId, const std::string &service) const = 0;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_TEXT_TRACK_WRAPPER_H_
