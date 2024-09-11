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

#include "SetSyncOff.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
SetSyncOff::SetSyncOff(IGstGenericPlayerPrivate &player,
                       const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                       const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper, bool syncOff)
    : m_player(player), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_syncOff{syncOff}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetSyncOff");
}

SetSyncOff::~SetSyncOff()
{
    RIALTO_SERVER_LOG_DEBUG("SetSyncOff finished");
}

void SetSyncOff::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetSyncOff");

    GstElement *decoder = m_player.getDecoder(MediaSourceType::AUDIO);
    if (decoder && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "sync-off"))
    {
        m_glibWrapper->gObjectSet(decoder, "sync-off", m_syncOff, nullptr);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set sync-off property on decoder '%s'",
                                (decoder ? GST_ELEMENT_NAME(decoder) : "null"));
    }

    if (decoder)
        m_gstWrapper->gstObjectUnref(GST_OBJECT(decoder));
}
} // namespace firebolt::rialto::server::tasks::generic
