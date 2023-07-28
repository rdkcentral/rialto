/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "tasks/generic/SetMute.h"
#include "RialtoServerLogging.h"
#include <gst/gst.h>
#include <gst/audio/streamvolume.h>

namespace firebolt::rialto::server::tasks::generic
{
SetMute::SetMute(GenericPlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper, bool mute)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_mute{mute}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetMute");
}

SetMute::~SetMute()
{
    RIALTO_SERVER_LOG_DEBUG("SetMute finished");
}

void SetMute::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetMute");
    if (!m_context.pipeline)
    {
        RIALTO_SERVER_LOG_ERROR("Setting mute failed. Pipeline is NULL");
        return;
    }
    m_gstWrapper->gstStreamVolumeSetMute(GST_STREAM_VOLUME(m_context.pipeline), m_mute);
}
} // namespace firebolt::rialto::server::tasks::generic
