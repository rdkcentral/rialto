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

#include "tasks/FinishSetupSource.h"
#include "IGstPlayerClient.h"
#include "IGstPlayerPrivate.h"
#include "PlayerContext.h"
#include "RialtoServerLogging.h"
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

namespace
{
/**
 * @brief Callback for need-data event from gstreamer. Called by the Gstreamer thread.
 *
 * @param[in] src       : the appsrc element that emitted the signal
 * @param[in] length    : the amount of bytes needed.
 * @param[in] user_data : The data to be passed with the message.
 *
 */
void appSrcNeedData(GstAppSrc *src, guint length, gpointer user_data)
{
    firebolt::rialto::server::IGstPlayerPrivate *self =
        static_cast<firebolt::rialto::server::IGstPlayerPrivate *>(user_data);
    self->scheduleNeedMediaData(src);
}

/**
 * @brief Callback for enough-data event from gstreamer. Called by the Gstreamer thread.
 *
 * @param[in] src       : the appsrc element that emitted the signal
 * @param[in] user_data : The data to be passed with the message.
 *
 */
void appSrcEnoughData(GstAppSrc *src, gpointer user_data)
{
    firebolt::rialto::server::IGstPlayerPrivate *self =
        static_cast<firebolt::rialto::server::IGstPlayerPrivate *>(user_data);
    self->scheduleEnoughData(src);
}

/**
 * @brief Callback for seek-data event from gstreamer. Called by the Gstreamer thread.
 *
 * @param[in] src       : the appsrc element that emitted the signal
 * @param[in] offset    : the offset to seek to
 * @param[in] user_data : The data to be passed with the message.
 *
 * @retval true if the handling of the message is successful, false otherwise.
 */
gboolean appSrcSeekData(GstAppSrc *src, guint64 offset, gpointer user_data)
{
    appSrcEnoughData(src, user_data);
    return TRUE;
}
} // namespace

namespace firebolt::rialto::server
{
FinishSetupSource::FinishSetupSource(PlayerContext &context, IGstPlayerPrivate &player, IGstPlayerClient *client)
    : m_context{context}, m_player{player}, m_gstPlayerClient{client}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing FinishSetupSource");
}

FinishSetupSource::~FinishSetupSource()
{
    RIALTO_SERVER_LOG_DEBUG("FinishSetupSource finished");
}

void FinishSetupSource::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing FinishSetupSource");
    GstAppSrcCallbacks callbacks = {appSrcNeedData, appSrcEnoughData, appSrcSeekData, {nullptr}};

    auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO);
    if (elem != m_context.streamInfo.end())
    {
        m_context.gstSrc->setupAndAddAppArc(m_context.source, elem->second, &callbacks, &m_player,
                                            firebolt::rialto::MediaSourceType::AUDIO);
        m_player.notifyNeedMediaData(true, false);
    }

    elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO);
    if (elem != m_context.streamInfo.end())
    {
        m_context.gstSrc->setupAndAddAppArc(m_context.source, elem->second, &callbacks, &m_player,
                                            firebolt::rialto::MediaSourceType::VIDEO);
        m_player.notifyNeedMediaData(false, true);
    }

    m_context.gstSrc->allAppSrcsAdded(m_context.source);

    // Notify GstPlayerClient of Idle state once setup has finished
    if (m_gstPlayerClient)
        m_gstPlayerClient->notifyPlaybackState(PlaybackState::IDLE);
}
} // namespace firebolt::rialto::server
