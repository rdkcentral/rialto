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

#include "tasks/generic/Eos.h"
#include "GenericPlayerContext.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::tasks::generic
{
Eos::Eos(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, std::shared_ptr<IGstWrapper> gstWrapper,
         const firebolt::rialto::MediaSourceType &type)
    : m_context{context}, m_player{player}, m_gstWrapper{gstWrapper}, m_type{type}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing Eos");
}

Eos::~Eos()
{
    RIALTO_SERVER_LOG_DEBUG("Eos finished");
}

void Eos::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing Eos");
    if (m_type == firebolt::rialto::MediaSourceType::AUDIO)
    {
        if (m_context.audioUnderflowOccured)
        {
            RIALTO_SERVER_LOG_DEBUG("Cancelling audio underflow in EOS procedure");
            m_player.cancelUnderflow(m_context.audioUnderflowOccured);
        }
        // Disable audio underflow notifications once EOS has been set on the source
        m_context.audioUnderflowEnabled = false;
    }
    else if (m_type == firebolt::rialto::MediaSourceType::VIDEO)
    {
        if (m_context.videoUnderflowOccured)
        {
            RIALTO_SERVER_LOG_DEBUG("Cancelling video underflow in EOS procedure");
            m_player.cancelUnderflow(m_context.videoUnderflowOccured);
        }
        // Disable video underflow notifications once EOS has been set on the source
        m_context.videoUnderflowEnabled = false;
    }
    auto elem = m_context.streamInfo.find(m_type);
    if (elem == m_context.streamInfo.end())
    {
        RIALTO_SERVER_LOG_WARN("Set eos failed - Stream not found");
        return;
    }
    if (m_gstWrapper->gstAppSrcEndOfStream(GST_APP_SRC(elem->second.appSrc)) != GST_FLOW_OK)
    {
        RIALTO_SERVER_LOG_WARN("Set eos failed - Gstreamer error");
    }
    else
    {
        m_context.endOfStreamInfo.emplace(m_type, elem->second);
    }
}
} // namespace firebolt::rialto::server::tasks::generic
