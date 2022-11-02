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

#include "tasks/Underflow.h"
#include "IGstPlayerClient.h"
#include "IGstPlayerPrivate.h"
#include "RialtoServerLogging.h"
#include "tasks/Pause.h"

namespace firebolt::rialto::server
{
Underflow::Underflow(IGstPlayerPrivate &player, IGstPlayerClient *client, bool &underflowFlag)
    : m_player{player}, m_gstPlayerClient{client}, m_underflowFlag{underflowFlag}
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
    if (m_underflowFlag)
    {
        return;
    }
    m_underflowFlag = true;
    Pause pauseTask{m_player};
    pauseTask.execute();
    if (m_gstPlayerClient)
    {
        m_gstPlayerClient->notifyNetworkState(NetworkState::STALLED);
    }
}
} // namespace firebolt::rialto::server
