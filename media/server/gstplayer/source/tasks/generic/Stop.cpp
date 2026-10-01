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

#include "tasks/generic/Stop.h"
#include "GenericPlayerContext.h"
#include "IGstGenericPlayerPrivate.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::tasks::generic
{
Stop::Stop(GenericPlayerContext &context, IGstGenericPlayerPrivate &player) : m_context{context}, m_player{player}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing Stop");
}

Stop::~Stop()
{
    RIALTO_SERVER_LOG_DEBUG("Stop finished");
}

void Stop::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing Stop");
    m_player.stopPositionReportingAndCheckAudioUnderflowTimer();
    RIALTO_SERVER_LOG_MIL("USHA: m_player.changePipelineState(GST_STATE_NULL)");
    m_player.changePipelineState(GST_STATE_NULL);
    for (auto &streamInfo : m_context.streamInfo)
    {
        streamInfo.second.isDataNeeded = false;
        RIALTO_SERVER_LOG_MIL("USHA: streamInfo.second.isDataNeeded = false");
    }
    RIALTO_SERVER_LOG_MIL("USHA: Checking on Stop() call");

    // TEMPORARY INSTRUMENTATION (CPESP-10589) - measure stale queued buffers at stop. Do not merge.
    {
        gsize totalBytes{0};
        size_t totalCount{0};
        for (const auto &elem : m_context.streamInfo)
        {
            gsize streamBytes{0};
            for (GstBuffer *buffer : elem.second.buffers)
            {
                streamBytes += gst_buffer_get_size(buffer);
            }
            totalBytes += streamBytes;
            totalCount += elem.second.buffers.size();
            RIALTO_SERVER_LOG_MIL("USHA: MEMSTAT: sourceType=%d count=%zu bytes=%" G_GSIZE_FORMAT " isDataNeeded=%d",
                                  static_cast<int>(elem.first), elem.second.buffers.size(), streamBytes,
                                  static_cast<int>(elem.second.isDataNeeded));
        }
        RIALTO_SERVER_LOG_MIL("MEMSTAT: TOTAL streams=%zu count=%zu bytes=%" G_GSIZE_FORMAT, m_context.streamInfo.size(),
                              totalCount, totalBytes);
    }

    RIALTO_SERVER_LOG_MIL("State change to NULL requested");
}
} // namespace firebolt::rialto::server::tasks::generic
