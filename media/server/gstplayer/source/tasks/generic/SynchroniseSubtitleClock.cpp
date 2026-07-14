/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "tasks/generic/SynchroniseSubtitleClock.h"
#include "IGstGenericPlayerPrivate.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"
#include <inttypes.h>

namespace firebolt::rialto::server::tasks::generic
{
SynchroniseSubtitleClock::SynchroniseSubtitleClock(
    GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
    const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper)
    : m_context{context}, m_player{player}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SynchroniseSubtitleClock");
}

SynchroniseSubtitleClock::~SynchroniseSubtitleClock()
{
    RIALTO_SERVER_LOG_DEBUG("SynchroniseSubtitleClock finished");
}

void SynchroniseSubtitleClock::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SynchroniseSubtitleClock");
    gint64 position = m_player.getPosition(m_context.pipeline);
    if (position == -1)
    {
        RIALTO_SERVER_LOG_WARN("Getting the position failed");
        return;
    }

    auto sourceElem = m_context.streamInfo.find(MediaSourceType::SUBTITLE);
    GstElement *source{nullptr};

    if (sourceElem != m_context.streamInfo.end())
    {
        source = sourceElem->second.appSrc;
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("subtitle source not found");
        return;
    }

    GstStructure *structure = m_gstWrapper->gstStructureNew("current-pts", "pts", G_TYPE_UINT64, position, nullptr);
    GstEvent *event = m_gstWrapper->gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, structure);

    if (event)
    {
        if (m_gstWrapper->gstElementSendEvent(source, event))
        {
            RIALTO_SERVER_LOG_DEBUG("Sent current-pts event to subtitlesource");
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to send current-pts event to source");
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create current-pts event");
        m_gstWrapper->gstStructureFree(structure);
    }
}

} // namespace firebolt::rialto::server::tasks::generic
