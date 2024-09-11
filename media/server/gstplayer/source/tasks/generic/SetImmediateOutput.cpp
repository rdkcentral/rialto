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

#include "SetImmediateOutput.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
SetImmediateOutput::SetImmediateOutput(IGstGenericPlayerPrivate &player,
                                       const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                       const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                                       const MediaSourceType &type, bool immediateOutput)
    : m_player(player), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_type{type}, m_immediateOutput{
                                                                                                immediateOutput}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetImmediateOutput");
}

SetImmediateOutput::~SetImmediateOutput()
{
    RIALTO_SERVER_LOG_DEBUG("SetImmediateOutput finished");
}

void SetImmediateOutput::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetImmediateOutput for %s source", common::convertMediaSourceType(m_type));

    GstElement *sink = m_player.getSink(m_type);
    if (sink)
    {
        // For AutoVideoSink we use properties on the child sink
        GstElement *actualSink{sink};
        if (MediaSourceType::VIDEO == m_type)
            actualSink = m_player.getSinkChildIfAutoVideoSink(sink);

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(actualSink), "immediate-output"))
        {
            gboolean immediateOutput{m_immediateOutput};
            m_glibWrapper->gObjectSet(actualSink, "immediate-output", immediateOutput, nullptr);
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set immediate-output property on sink '%s'", GST_ELEMENT_NAME(actualSink));
        }
        m_gstWrapper->gstObjectUnref(GST_OBJECT(sink));
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set immediate-output property, sink is NULL");
    }
}
} // namespace firebolt::rialto::server::tasks::generic
