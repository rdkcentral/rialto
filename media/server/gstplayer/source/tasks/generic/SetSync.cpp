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

#include "SetSync.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
SetSync::SetSync(IGstGenericPlayerPrivate &player,
                 const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                 const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper, bool sync)
    : m_player(player), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_sync{sync}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetSync");
}

SetSync::~SetSync()
{
    RIALTO_SERVER_LOG_DEBUG("SetSync finished");
}

void SetSync::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetSync");

    GstElement *sink = m_player.getSink(MediaSourceType::AUDIO);
    GstElement *actualSink = getSinkChildIfAutoAudioSink(sink);
    if (actualSink && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(actualSink), "sync"))
    {
        m_glibWrapper->gObjectSet(actualSink, "sync", m_sync, nullptr);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set sync property on sink '%s'", (actualSink ? GST_ELEMENT_NAME(actualSink) : "null"));
    }

    if (sink)
        m_gstWrapper->gstObjectUnref(GST_OBJECT(sink));
}
} // namespace firebolt::rialto::server::tasks::generic
