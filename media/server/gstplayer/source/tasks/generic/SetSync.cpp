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

#include "SetSync.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::tasks::generic
{
SetSync::SetSync(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, bool sync)
    : m_context(context), m_player(player), m_sync{sync}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetSync");
}

SetSync::~SetSync()
{
    RIALTO_SERVER_LOG_DEBUG("SetSync finished");
}

void SetSync::execute() const
{
    m_context.pendingSync = m_sync;
    if (m_context.pipeline)
    {
        m_player.setSync();
    }
}
} // namespace firebolt::rialto::server::tasks::generic
