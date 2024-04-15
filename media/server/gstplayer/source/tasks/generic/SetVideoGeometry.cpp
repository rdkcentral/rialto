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

#include "tasks/generic/SetVideoGeometry.h"
#include "IGstGenericPlayerPrivate.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::tasks::generic
{
SetVideoGeometry::SetVideoGeometry(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                   const Rectangle &rectangle)
    : m_context{context}, m_player{player}, m_rectangle{rectangle}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetVideoGeometry");
}

SetVideoGeometry::~SetVideoGeometry()
{
    RIALTO_SERVER_LOG_DEBUG("SetVideoGeometry finished");
}

void SetVideoGeometry::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetVideoGeometry");
    m_context.pendingGeometry = m_rectangle;
    if (m_context.pipeline)
    {
        m_player.setVideoSinkRectangle();
    }
}
} // namespace firebolt::rialto::server::tasks::generic
