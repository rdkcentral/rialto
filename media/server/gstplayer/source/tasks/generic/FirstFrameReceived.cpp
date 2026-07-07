/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "tasks/generic/FirstFrameReceived.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
FirstFrameReceived::FirstFrameReceived(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                       IGstGenericPlayerClient *client, MediaSourceType sourceType,
                                       AudioFirstFrameAction audioAction)
    : m_context{context}, m_player{player}, m_gstPlayerClient{client}, m_sourceType{sourceType},
      m_audioAction{audioAction}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing FirstFrameReceived");
}

FirstFrameReceived::~FirstFrameReceived()
{
    RIALTO_SERVER_LOG_DEBUG("FirstFrameReceived finished");
}

void FirstFrameReceived::execute() const
{
    RIALTO_SERVER_LOG_WARN("Executing FirstFrameReceived for %s source", common::convertMediaSourceType(m_sourceType));

    if (m_sourceType == MediaSourceType::AUDIO)
    {
        if (m_audioAction == AudioFirstFrameAction::CLEAR_PROBE)
        {
            m_player.clearAudioFirstFrameFallbackProbe();
        }
        else if (m_audioAction == AudioFirstFrameAction::CLEAR_PROBE_STATE)
        {
            m_player.clearAudioFirstFrameFallbackProbeState();
        }

        if (m_context.audioSourceRemoved)
        {
            RIALTO_SERVER_LOG_DEBUG("Ignoring first audio frame notification - audio source removed");
            return;
        }

        if (m_context.firstAudioFrameReceived)
        {
            RIALTO_SERVER_LOG_DEBUG("First audio frame notification already sent");
            return;
        }

        m_context.firstAudioFrameReceived = true;
    }

    if (m_gstPlayerClient)
    {
        m_gstPlayerClient->notifyFirstFrameReceived(m_sourceType);
    }
}
} // namespace firebolt::rialto::server::tasks::generic
