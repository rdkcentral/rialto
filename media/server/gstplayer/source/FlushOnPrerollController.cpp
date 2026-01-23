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

#include "FlushOnPrerollController.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"
#include <gst/gst.h>

namespace firebolt::rialto::server
{
void FlushOnPrerollController::waitIfRequired(const MediaSourceType &type)
{
    std::unique_lock lock{m_mutex};
    RIALTO_SERVER_LOG_MIL("Waiting if required for %s source entry", common::convertMediaSourceType(type));
    m_conditionVariable.wait(lock, [this, &type]()
                             { return !m_isPrerolled || m_flushingSources.find(type) == m_flushingSources.end(); });
    RIALTO_SERVER_LOG_MIL("Waiting if required for %s source exit", common::convertMediaSourceType(type));
    // return m_isPrerolled && m_flushingSources.find(type) != m_flushingSources.end();
}

void FlushOnPrerollController::setFlushing(const MediaSourceType &type, const GstState &currentPipelineState)
{
    RIALTO_SERVER_LOG_MIL("Set flushing for: %s, state: %s", common::convertMediaSourceType(type),
                          gst_element_state_get_name(currentPipelineState));
    std::unique_lock lock{m_mutex};
    m_flushingSources.insert(type);
    m_isPrerolled = false;
    if (!m_targetState.has_value())
    {
        m_targetState = currentPipelineState;
    }
}

void FlushOnPrerollController::stateReached(const GstState &newPipelineState)
{
    RIALTO_SERVER_LOG_MIL("State reached %s", gst_element_state_get_name(newPipelineState));
    std::unique_lock lock{m_mutex};
    m_isPrerolled = true;
    if (m_targetState.has_value() && newPipelineState == m_targetState.value())
    {
        m_flushingSources.clear();
        m_targetState = std::nullopt;
    }
    m_conditionVariable.notify_all();
}

void FlushOnPrerollController::reset()
{
    RIALTO_SERVER_LOG_MIL("Reset");
    std::unique_lock lock{m_mutex};
    m_isPrerolled = false;
    m_flushingSources.clear();
    m_targetState = std::nullopt;
    m_conditionVariable.notify_all();
}
} // namespace firebolt::rialto::server
