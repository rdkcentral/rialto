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

#include "tasks/SetPosition.h"
#include "IGstPlayerClient.h"
#include "IGstPlayerPrivate.h"
#include "IGstWrapper.h"
#include "PlayerContext.h"
#include "RialtoServerLogging.h"
#include "tasks/NeedData.h"

namespace firebolt::rialto::server
{
SetPosition::SetPosition(PlayerContext &context, IGstPlayerPrivate &player, IGstPlayerClient *client,
                         std::shared_ptr<IGstWrapper> gstWrapper, std::int64_t position)
    : m_context{context}, m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}, m_position{position}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetPosition");
}

SetPosition::~SetPosition()
{
    RIALTO_SERVER_LOG_DEBUG("SetPosition finished");
}

void SetPosition::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetPosition");
    if (!m_gstPlayerClient)
    {
        RIALTO_SERVER_LOG_ERROR("Seek failed - GstPlayerClient is NULL");
        return;
    }
    m_gstPlayerClient->notifyPlaybackState(PlaybackState::SEEKING);

    // Stop sending new NeedMediaData requests
    m_context.audioNeedData = false;
    m_context.videoNeedData = false;

    // Clear local cache of any active data requests for player session
    m_context.audioNeedDataPending = false;
    m_context.videoNeedDataPending = false;
    m_gstPlayerClient->clearActiveRequestsCache();

    // Clear buffered samples for player session
    for (auto &buffer : m_context.audioBuffers)
    {
        m_gstWrapper->gstBufferUnref(buffer);
    }
    m_context.audioBuffers.clear();
    for (auto &buffer : m_context.videoBuffers)
    {
        m_gstWrapper->gstBufferUnref(buffer);
    }
    m_context.videoBuffers.clear();

    m_context.lastAudioSampleTimestamps = m_position;

    if (!m_context.pipeline)
    {
        RIALTO_SERVER_LOG_ERROR("Seek failed - pipeline is null");
        m_gstPlayerClient->notifyPlaybackState(PlaybackState::FAILURE);
        return;
    }
    if (!m_gstWrapper->gstElementSeek(m_context.pipeline, m_context.playbackRate, GST_FORMAT_TIME,
                                      static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_SET, m_position,
                                      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
    {
        RIALTO_SERVER_LOG_ERROR("Seek failed - gstreamer error");
        m_gstPlayerClient->notifyPlaybackState(PlaybackState::FAILURE);
        return;
    }

    m_gstPlayerClient->notifyPlaybackState(PlaybackState::FLUSHED);

    // // Trigger NeedMediaData for all attached sources
    for (const auto streamInfo : m_context.streamInfo)
    {
        if (streamInfo.second)
        {
            NeedData task{m_context, m_gstPlayerClient, GST_APP_SRC(streamInfo.second)};
            task.execute();
        }
    }
}
} // namespace firebolt::rialto::server
