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

#include "tasks/generic/Underflow.h"
#include "IGstGenericPlayerClient.h"
#include "IGstGenericPlayerPrivate.h"
#include "RialtoServerLogging.h"
#include "tasks/generic/Pause.h"

namespace firebolt::rialto::server::tasks::generic
{
Underflow::Underflow(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, IGstGenericPlayerClient *client, bool &underflowFlag, bool underflowEnabled)
    : m_context{context}, m_player{player}, m_gstPlayerClient{client}, m_underflowFlag{underflowFlag}, m_underflowEnabled{underflowEnabled}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing Underflow");
}

Underflow::~Underflow()
{
    RIALTO_SERVER_LOG_DEBUG("Underflow finished");
}

void Underflow::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing Underflow");
    if (!m_underflowEnabled)
    {
        return;
    }
    if (m_underflowFlag)
    {
        return;
    }
    m_underflowFlag = true;

    // If the EOS has been raised, notify EndOfStream, not underflow.
    if (!m_context.eosNotified && allSourcesEos())
    {
        RIALTO_SERVER_LOG_WARN("Received underflow, but all streams are ended, so reporting EOS.");
        if (m_gstPlayerClient)
        {
            m_gstPlayerClient->notifyPlaybackState(PlaybackState::END_OF_STREAM);
            m_context.eosNotified = true;
        }
    }
    else
    {
        Pause pauseTask{m_context, m_player};
        pauseTask.execute();
        if (m_gstPlayerClient)
        {
            m_gstPlayerClient->notifyNetworkState(NetworkState::STALLED);
        }
    }
}

bool Underflow::allSourcesEos() const
{
    for (const auto &streamInfo : m_context.streamInfo)
    {
        if (m_context.endOfStreamInfo.find(streamInfo.first) == m_context.endOfStreamInfo.end())
        {
            return false;
        }
    }
    return true;
}
} // namespace firebolt::rialto::server::tasks::generic
