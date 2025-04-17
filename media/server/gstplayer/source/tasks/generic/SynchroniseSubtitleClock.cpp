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
    RIALTO_SERVER_LOG_ERROR("KLOPS Executing SynchroniseSubtitleClock");
    if (m_context.videoSink)
    {
        GstClockTime gstVideoPts = 0;
        gint64 videoPts = 0;
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(m_context.videoSink), "video-pts"))
        {
            m_glibWrapper->gObjectGet(m_context.videoSink, "video-pts", &videoPts, nullptr);
            gstVideoPts = GST_TIME_AS_NSECONDS(videoPts) * GST_SECOND / 90000;
            RIALTO_SERVER_LOG_ERROR("KLOPS Raw Video PTS: %" PRIu64 ", Video PTS: %" GST_TIME_FORMAT, videoPts,
                                    GST_TIME_ARGS(gstVideoPts));
        }
        else
        {
            RIALTO_SERVER_LOG_WARN("KLOPS video-pts property not found in video-sink");
        }
        std::int64_t position2 = 0;
        if (m_gstWrapper->gstElementQueryPosition(m_context.videoSink, GST_FORMAT_TIME, &position2))
        {
            // Create and send a custom event with the current PTS value
            GstStructure *structure =
                m_gstWrapper->gstStructureNew("current-pts", "pts", G_TYPE_INT64, gstVideoPts, nullptr);
            GstEvent *event = m_gstWrapper->gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM, structure);
            if (event)
            {
                RIALTO_SERVER_LOG_ERROR("KLOPS Sending current-pts event with value: %" GST_TIME_FORMAT,
                                        GST_TIME_ARGS(gstVideoPts));
                if (!m_gstWrapper->gstElementSendEvent(m_context.subtitleSink, event))
                {
                    RIALTO_SERVER_LOG_ERROR("KLOPS Failed to send current-pts event to subtitle sink");
                }
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("KLOPS Failed to create current-pts event");
            }
            m_glibWrapper->gObjectSet(m_context.subtitleSink, "test", gstVideoPts, nullptr);
            RIALTO_SERVER_LOG_ERROR("KLOPS Sink position: %" GST_TIME_FORMAT, GST_TIME_ARGS(position2));
        }
        RIALTO_SERVER_LOG_ERROR("KLOPS position and PTS equal: %d", (GstClockTime)position2 == gstVideoPts);
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("KLOPS video-sink is NULL");
    }

    RIALTO_SERVER_LOG_ERROR("KLOPS ************************************ KLOPS ************************************");
}

} // namespace firebolt::rialto::server::tasks::generic
