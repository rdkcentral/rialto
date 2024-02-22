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

#include "tasks/generic/Flush.h"
#include "RialtoServerLogging.h"
#include "tasks/generic/NeedData.h"

namespace firebolt::rialto::server::tasks::generic
{
Flush::Flush(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, IGstGenericPlayerClient *client,
             std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper, const MediaSourceType &type,
             bool resetTime)
    : m_context{context}, m_player{player}, m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}, m_type{type},
      m_resetTime{resetTime}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing Flush");
}

Flush::~Flush()
{
    RIALTO_SERVER_LOG_DEBUG("Flush finished");
}

void Flush::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing Flush");

    // Get source first
    GstElement *source{nullptr};
    auto sourceElem = m_context.streamInfo.find(m_type);
    if (sourceElem != m_context.streamInfo.end())
    {
        source = sourceElem->second.appSrc;
    }
    if (!source)
    {
        RIALTO_SERVER_LOG_WARN("failed to flush - source is NULL");
        return;
    }

    // Clear/invalidate old NeedDatas
    switch (m_type)
    {
    case MediaSourceType::AUDIO:
    {
        m_context.audioNeedData = false;
        m_context.audioNeedDataPending = false;
        for (auto &buffer : m_context.audioBuffers)
        {
            m_gstWrapper->gstBufferUnref(buffer);
        }
        m_context.audioBuffers.clear();
        break;
    }
    case MediaSourceType::VIDEO:
    {
        m_context.videoNeedData = false;
        m_context.videoNeedDataPending = false;
        for (auto &buffer : m_context.videoBuffers)
        {
            m_gstWrapper->gstBufferUnref(buffer);
        }
        m_context.videoBuffers.clear();
        break;
    }
    case MediaSourceType::UNKNOWN:
    default:
    {
        RIALTO_SERVER_LOG_WARN("Flush failed: Media source type not supported.");
        return;
    }
    }
    m_gstPlayerClient->invalidateActiveRequests(m_type);

    // Query current segment, if we don't reset time
    std::int64_t position{0};
    double rate{1.0};
    gint64 stop{-1};
    if (!m_resetTime)
    {
        GstFormat format{GST_FORMAT_UNDEFINED};
        gint64 start{0};
        m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &position);
        GstQuery *query{m_gstWrapper->gstQueryNewSegment(GST_FORMAT_TIME)};
        if (m_gstWrapper->gstElementQuery(m_context.pipeline, query))
        {
            m_gstWrapper->gstQueryParseSegment(query, &rate, &format, &start, &stop);
        }
        m_gstWrapper->gstQueryUnref(query);
    }

    // Flush source
    GstEvent *flushStart = m_gstWrapper->gstEventNewFlushStart();
    if (!m_gstWrapper->gstElementSendEvent(source, flushStart))
    {
        RIALTO_SERVER_LOG_WARN("failed to send flush-start event");
    }

    // Update segment if we don't reset time
    if (!m_resetTime)
    {
        GstSegment *segment{m_gstWrapper->gstSegmentNew()};
        m_gstWrapper->gstSegmentInit(segment, GST_FORMAT_TIME);
        m_gstWrapper->gstSegmentDoSeek(segment, rate, GST_FORMAT_TIME, GST_SEEK_FLAG_NONE, GST_SEEK_TYPE_SET, position,
                                       GST_SEEK_TYPE_SET, stop, nullptr);

        RIALTO_SERVER_LOG_INFO("Set new seamless segment: [%" GST_TIME_FORMAT ", %" GST_TIME_FORMAT "], rate: %f \n",
                               GST_TIME_ARGS(segment->start), GST_TIME_ARGS(segment->stop), segment->rate);

        if (!m_gstWrapper->gstBaseSrcNewSeamlessSegment(GST_BASE_SRC(source), segment->start, segment->stop,
                                                        segment->start))
        {
            RIALTO_SERVER_LOG_WARN("Failed to set seamless segment event");
        }
        m_gstWrapper->gstSegmentFree(segment);
    }

    GstEvent *flushStop = m_gstWrapper->gstEventNewFlushStop(m_resetTime);
    if (!m_gstWrapper->gstElementSendEvent(source, flushStop))
    {
        RIALTO_SERVER_LOG_WARN("failed to send flush-stop event");
    }

    // Notify client, that flush has been finished
    m_gstPlayerClient->notifySourceFlushed(m_type);

    // Trigger NeedData for flushed source
    NeedData task{m_context, m_gstPlayerClient, GST_APP_SRC(source)};
    task.execute();
}
} // namespace firebolt::rialto::server::tasks::generic
