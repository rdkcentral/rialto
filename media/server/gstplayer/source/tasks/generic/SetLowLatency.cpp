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

#include "SetLowLatency.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
SetLowLatency::SetLowLatency(IGstGenericPlayerPrivate &player,
                                       const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                       const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                                       bool lowLatency)
    : m_player(player), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_lowLatency{lowLatency}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetLowLatency");
}

SetLowLatency::~SetLowLatency()
{
    RIALTO_SERVER_LOG_DEBUG("SetLowLatency finished");
}

void SetLowLatency::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetLowLatency");

    GstElement *sink = m_player.getSink(MediaSourceType::AUDIO);
    if (sink && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "low-latency"))
    {
        // TODO: For AutoVideoSink we use properties on the child sink
        m_glibWrapper->gObjectSet(sink, "low-latency", m_lowLatency, nullptr);
        m_gstWrapper->gstObjectUnref(GST_OBJECT(sink));
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set low-latency property on sink '%s'", (sink ? GST_ELEMENT_NAME(sink) : "null"));
    }
}
} // namespace firebolt::rialto::server::tasks::generic
