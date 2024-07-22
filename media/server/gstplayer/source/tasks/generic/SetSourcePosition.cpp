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

#include "SetSourcePosition.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"
#include "tasks/generic/NeedData.h"

namespace firebolt::rialto::server::tasks::generic
{
SetSourcePosition::SetSourcePosition(GenericPlayerContext &context, IGstGenericPlayerClient *client,
                                     const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                     const MediaSourceType &type, std::int64_t position)
    : m_context{context}, m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}, m_type{type}, m_position{position}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetSourcePosition");
}

SetSourcePosition::~SetSourcePosition()
{
    RIALTO_SERVER_LOG_DEBUG("SetSourcePosition finished");
}

void SetSourcePosition::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetSourcePosition for %s source", common::convertMediaSourceType(m_type));

    if (MediaSourceType::UNKNOWN == m_type)
    {
        RIALTO_SERVER_LOG_WARN("failed to set source position - source type is unknown");
        return;
    }

    // Get source first
    GstElement *source{nullptr};
    auto sourceElem = m_context.streamInfo.find(m_type);
    if (sourceElem != m_context.streamInfo.end())
    {
        source = sourceElem->second.appSrc;
    }
    if (!source)
    {
        RIALTO_SERVER_LOG_WARN("failed to set source position - %s source is NULL",
                               common::convertMediaSourceType(m_type));
        return;
    }

    m_context.initialPositions[source] = m_position;

    if (m_context.setupSourceFinished)
    {
        // Reset Eos info
        m_context.endOfStreamInfo.erase(m_type);
        m_context.eosNotified = false;

        // Trigger NeedData for source
        NeedData task{m_context, m_gstPlayerClient, GST_APP_SRC(source)};
        task.execute();
    }
}
} // namespace firebolt::rialto::server::tasks::generic
