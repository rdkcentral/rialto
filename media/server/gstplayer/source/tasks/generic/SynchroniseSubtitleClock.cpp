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

#include "tasks/generic/SynchroniseSubtitleClock.h"
#include "IGstGenericPlayerPrivate.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"
#include <inttypes.h>

namespace firebolt::rialto::server::tasks::generic
{
SynchroniseSubtitleClock::SynchroniseSubtitleClock(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                   std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
                                                   std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper)
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
    if (m_context.videoSink)
    {
        GstClockTime gstVideoPts = 0;
        gint64 videoPts = 0;
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(m_context.videoSink), "video-pts"))
        {
            m_glibWrapper->gObjectGet(m_context.videoSink, "video-pts", &videoPts, nullptr);
            // videoPts is in 90kHz ticks
            gstVideoPts = GST_TIME_AS_NSECONDS(videoPts) * GST_SECOND / 90000;
        }
        else
        {
            RIALTO_SERVER_LOG_WARN("video-pts property not found in video-sink");
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

        GstStructure *structure =
            m_gstWrapper->gstStructureNew("current-pts", "pts", G_TYPE_UINT64, gstVideoPts, nullptr);
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
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("video-sink is NULL");
    }
}

} // namespace firebolt::rialto::server::tasks::generic
