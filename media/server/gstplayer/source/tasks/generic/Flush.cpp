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

namespace firebolt::rialto::server::tasks::generic
{
Flush::Flush(GenericPlayerContext &context, IGstGenericPlayerClient *client,
             std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper, const MediaSourceType &type,
             bool resetTime)
    : m_context{context}, m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}, m_type{type}, m_resetTime{resetTime}
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

    if (GST_STATE(m_context.pipeline) >= GST_STATE_PAUSED)
    {
        // Flush source
        GstEvent *flushStart = m_gstWrapper->gstEventNewFlushStart();
        if (!m_gstWrapper->gstElementSendEvent(source, flushStart))
        {
            RIALTO_SERVER_LOG_WARN("failed to send flush-start event");
        }

        GstEvent *flushStop = m_gstWrapper->gstEventNewFlushStop(m_resetTime);
        if (!m_gstWrapper->gstElementSendEvent(source, flushStop))
        {
            RIALTO_SERVER_LOG_WARN("failed to send flush-stop event");
        }
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Skip sending flush event - pipeline below paused");
    }

    // Reset Eos info
    m_context.endOfStreamInfo.erase(m_type);
    m_context.eosNotified = false;

    // Notify client, that flush has been finished
    m_gstPlayerClient->notifySourceFlushed(m_type);
}
} // namespace firebolt::rialto::server::tasks::generic
