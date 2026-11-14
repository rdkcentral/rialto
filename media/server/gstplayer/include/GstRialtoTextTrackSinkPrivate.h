/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_GST_RIALTO_TEXT_TRACK_SINK_PRIVATE_H_
#define FIREBOLT_RIALTO_SERVER_GST_RIALTO_TEXT_TRACK_SINK_PRIVATE_H_

#include "ITextTrackSession.h"
#include <atomic>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace firebolt::rialto::server
{
inline uint64_t calculateSubtitlePosition(uint64_t position, int64_t offset)
{
    if (offset < 0)
    {
        const uint64_t offsetMagnitude = static_cast<uint64_t>(-(offset + 1)) + 1;
        return position < offsetMagnitude ? 0 : position - offsetMagnitude;
    }

    const uint64_t positiveOffset = static_cast<uint64_t>(offset);
    if (position > std::numeric_limits<uint64_t>::max() - positiveOffset)
    {
        return std::numeric_limits<uint64_t>::max();
    }

    return position + positiveOffset;
}

struct GstRialtoTextTrackSinkPrivate
{
    std::unique_ptr<ITextTrackSession> m_textTrackSession;
    std::atomic<bool> m_isMuted{false};
    std::string m_textTrackIdentifier;
    gpointer m_videoDecoderIdentifier{nullptr};
    bool m_capsSet{false};
    std::optional<uint64_t> m_position;
    std::optional<uint64_t> m_queuedPosition;
    std::optional<int64_t> m_offset;
    std::optional<int64_t> m_queuedOffset;
    std::mutex m_mutex;
};
}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_RIALTO_TEXT_TRACK_SINK_PRIVATE_H_
