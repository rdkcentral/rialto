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
SetSourcePosition::SetSourcePosition(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                     IGstGenericPlayerClient *client,
                                     const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                     const MediaSourceType &type, std::int64_t position, bool resetTime)
    : m_context{context},
      m_player(player), m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}, m_type{type}, m_position{position}, m_resetTime{resetTime}
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

    if (MediaSourceType::VIDEO == m_type || MediaSourceType::AUDIO == m_type)
    {
        m_context.initialPositions[source].emplace_back(SegmentData{m_position, m_resetTime});
    }

    if (m_context.setupSourceFinished)
    {
        if (MediaSourceType::SUBTITLE == m_type)
        {
            // in case of subtitles, all data might be already in the sink and we might not get any data anymore,
            // so send the new segment here and to not depend on any following buffers
            setSubtitlePosition();
        }

        // Reset Eos info
        m_context.endOfStreamInfo.erase(m_type);
        m_context.eosNotified = false;

        // Trigger NeedData for source
        NeedData task{m_context, m_gstPlayerClient, GST_APP_SRC(source)};
        task.execute();
    }
}

void SetSourcePosition::setSubtitlePosition() const
{
    GstSegment *segment{m_gstWrapper->gstSegmentNew()};
    m_gstWrapper->gstSegmentInit(segment, GST_FORMAT_TIME);
    if (!m_gstWrapper->gstSegmentDoSeek(segment, m_context.playbackRate, GST_FORMAT_TIME, GST_SEEK_FLAG_NONE,
                                        GST_SEEK_TYPE_SET, m_position, GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE, nullptr))
    {
        RIALTO_SERVER_LOG_WARN("Segment seek failed.");
        m_gstWrapper->gstSegmentFree(segment);
        return;
    }

    if (!m_gstWrapper->gstPadSendEvent(GST_BASE_SINK_PAD(m_context.subtitleSink),
                                       m_gstWrapper->gstEventNewSegment(segment)))
    {
        RIALTO_SERVER_LOG_WARN("Failed to new segment to subtitle sink");
    }

    m_gstWrapper->gstSegmentFree(segment);
}

} // namespace firebolt::rialto::server::tasks::generic
