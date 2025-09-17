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

#include "SetSubtitleOffset.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
SetSubtitleOffset::SetSubtitleOffset(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                     IGstGenericPlayerClient *client,
                                     const std::shared_ptr<wrappers::IGlibWrapper> &glibWrapper,
                                     const MediaSourceType &type, std::int64_t position)
    : m_context{context}, m_player(player), m_gstPlayerClient{client}, m_glibWrapper{glibWrapper}, m_type{type},
      m_position{position}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetSubtitleOffset");
}

SetSubtitleOffset::~SetSubtitleOffset()
{
    RIALTO_SERVER_LOG_DEBUG("SetSubtitleOffset finished");
}

void SetSubtitleOffset::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetSubtitleOffset for %s source", common::convertMediaSourceType(m_type));

    if (MediaSourceType::SUBTITLE != m_type)
    {
        RIALTO_SERVER_LOG_WARN("failed to set subtitle offset - source type is not subtitle");
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
        RIALTO_SERVER_LOG_WARN("failed to set subtitle offset - subtitle source is NULL");
        return;
    }

    // Set subtitle offset directly on the subtitle sink
    if (m_context.subtitleSink)
    {
        RIALTO_SERVER_LOG_DEBUG("Setting subtitle offset to %ld nanoseconds", m_position);
        m_glibWrapper->gObjectSet(m_context.subtitleSink, "subtitle-offset", static_cast<gint64>(m_position), nullptr);
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("failed to set subtitle offset - subtitle sink is NULL");
    }
}

} // namespace firebolt::rialto::server::tasks::generic