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

#include "tasks/generic/CheckAudioUnderflow.h"
#include "GenericPlayerContext.h"
#include "IGstGenericPlayerClient.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"
#include "tasks/generic/Underflow.h"
#include <gst/gst.h>

#include <cinttypes>

namespace firebolt::rialto::server
{
CheckAudioUnderflow::CheckAudioUnderflow(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                         IGstGenericPlayerClient *client, std::shared_ptr<IGstWrapper> gstWrapper)
    : m_context{context}, m_player(player), m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}
{
}

void CheckAudioUnderflow::execute() const
{
    // TODO(LLDEV-31012) Check if the audio stream is in underflow state.
    if (m_context.audioAppSrc)
    {
        gint64 position = -1;
        m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &position);
        constexpr int64_t kAudioUnderflowMarginNs = 350 * 1000000;
        if ((position > m_context.lastAudioSampleTimestamps + kAudioUnderflowMarginNs) &&
            m_gstWrapper->gstElementGetState(m_context.pipeline) == GST_STATE_PLAYING &&
            m_gstWrapper->gstElementGetPendingState(m_context.pipeline) != GST_STATE_PAUSED)
        {
            RIALTO_SERVER_LOG_INFO("Audio stream underflow! Position %" PRIu64 ", lastAudioSampleTimestamps: %" PRIu64,
                                   position, m_context.lastAudioSampleTimestamps);
            Underflow task(m_player, m_gstPlayerClient, m_context.audioUnderflowOccured);
            task.execute();
        }
    }
}

} // namespace firebolt::rialto::server
