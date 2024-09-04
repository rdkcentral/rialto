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

#include "SetStreamSyncMode.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
SetStreamSyncMode::SetStreamSyncMode(IGstGenericPlayerPrivate &player,
                                       const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                       const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                                       int32_t streamSyncMode)
    : m_player(player), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_streamSyncMode{streamSyncMode}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetStreamSyncMode");
}

SetStreamSyncMode::~SetStreamSyncMode()
{
    RIALTO_SERVER_LOG_DEBUG("SetStreamSyncMode finished");
}

void SetStreamSyncMode::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetStreamSyncMode");

    GstElement *decoder = m_player.getDecoder(MediaSourceType::AUDIO);
    if (decoder && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "stream-sync-mode"))
    {
        // TODO: For AutoVideoSink we use properties on the child sink
        m_glibWrapper->gObjectSet(decoder, "stream-sync-mode", m_streamSyncMode, nullptr);
        m_gstWrapper->gstObjectUnref(GST_OBJECT(decoder));
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set stream-sync-mode property on decoder '%s'", (decoder ? GST_ELEMENT_NAME(decoder) : "null"));
    }
}
} // namespace firebolt::rialto::server::tasks::generic