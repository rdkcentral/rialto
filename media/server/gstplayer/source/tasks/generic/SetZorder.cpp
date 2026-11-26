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

#include "SetZorder.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
SetZorder::SetZorder(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, const MediaSourceType &type,
                     float zorder)
    : m_context{context}, m_player{player}, m_type{type}, m_zorder{zorder}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetZorder");
}

SetZorder::~SetZorder()
{
    RIALTO_SERVER_LOG_DEBUG("SetZorder finished");
}

void SetZorder::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetZorder for %s source", common::convertMediaSourceType(m_type));

    if (MediaSourceType::VIDEO != m_type)
    {
        RIALTO_SERVER_LOG_ERROR("SetZorder not supported for source type %s", common::convertMediaSourceType(m_type));
        return;
    }

    m_context.pendingZorderForVideo = m_zorder;
    if (!m_context.pipeline)
    {
        RIALTO_SERVER_LOG_DEBUG("Pending zorder: pipeline is NULL");
        return;
    }

    m_player.setZorder();
}
} // namespace firebolt::rialto::server::tasks::generic
