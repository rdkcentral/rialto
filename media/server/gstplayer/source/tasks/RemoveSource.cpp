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

#include "tasks/RemoveSource.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
RemoveSource::RemoveSource(PlayerContext &context, IGstPlayerClient *client, const MediaSourceType &type)
    : m_context{context}, m_gstPlayerClient{client}, m_type{type}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing RemoveSource");
}

RemoveSource::~RemoveSource()
{
    RIALTO_SERVER_LOG_DEBUG("RemoveSource finished");
}

void RemoveSource::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing RemoveSource");
    if (MediaSourceType::AUDIO == m_type)
    {
        m_context.audioNeedData = false;
        m_context.audioNeedDataPending = false;
        m_context.audioUnderflowEnabled = false;
        m_context.audioSourceRemoved = true;
    }
    else if (MediaSourceType::VIDEO == m_type)
    {
        m_context.videoNeedData = false;
        m_context.videoNeedDataPending = false;
        m_context.videoUnderflowEnabled = false;
        m_context.videoSourceRemoved = true;
    }
    m_gstPlayerClient->invalidateActiveRequests(m_type);
}
} // namespace firebolt::rialto::server
