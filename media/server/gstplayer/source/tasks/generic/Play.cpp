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

#include "tasks/generic/Play.h"
#include "IGstGenericPlayerPrivate.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
Play::Play(IGstGenericPlayerPrivate &player) : m_player{player}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing Play");
}

Play::~Play()
{
    RIALTO_SERVER_LOG_DEBUG("Play finished");
}

void Play::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing Play");
    m_player.changePipelineState(GST_STATE_PLAYING);
}
} // namespace firebolt::rialto::server
