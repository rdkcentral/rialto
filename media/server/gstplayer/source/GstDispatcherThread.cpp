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

#include "GstDispatcherThread.h"
#include "IGstWrapper.h"
#include "IWorkerThread.h"
#include "PlayerContext.h"
#include "RialtoServerLogging.h"
#include "tasks/IPlayerTask.h"
#include "tasks/IPlayerTaskFactory.h"

namespace firebolt::rialto::server
{
std::unique_ptr<IGstDispatcherThread> GstDispatcherThreadFactory::createGstDispatcherThread(
    PlayerContext &playerContext, IGstPlayerPrivate &player, const std::shared_ptr<IGstWrapper> &gstWrapper,
    IWorkerThread &workerThread, const IPlayerTaskFactory &taskFactory) const
{
    return std::make_unique<GstDispatcherThread>(playerContext, player, gstWrapper, workerThread, taskFactory);
}

GstDispatcherThread::GstDispatcherThread(PlayerContext &playerContext, IGstPlayerPrivate &player,
                                         const std::shared_ptr<IGstWrapper> &gstWrapper, IWorkerThread &workerThread,
                                         const IPlayerTaskFactory &taskFactory)
    : m_context{playerContext}, m_player{player}, m_gstWrapper{gstWrapper}, m_workerThread{workerThread},
      m_kTaskFactory{taskFactory}, m_isGstreamerDispatcherActive{true}
{
    RIALTO_SERVER_LOG_INFO("GstDispatcherThread is starting");
    m_gstBusDispatcherThread = std::thread(&GstDispatcherThread::gstBusEventHandler, this, m_context.pipeline);
}

GstDispatcherThread::~GstDispatcherThread()
{
    RIALTO_SERVER_LOG_INFO("Stopping GstDispatcherThread");
    m_isGstreamerDispatcherActive = false;
    if (m_gstBusDispatcherThread.joinable())
    {
        m_gstBusDispatcherThread.join();
    }
}

void GstDispatcherThread::gstBusEventHandler(GstElement *pipeline)
{
    GstBus *bus = m_gstWrapper->gstPipelineGetBus(GST_PIPELINE(m_context.pipeline));
    if (!bus)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get gst bus");
        return;
    }

    while (m_isGstreamerDispatcherActive)
    {
        GstMessage *message =
            m_gstWrapper->gstBusTimedPopFiltered(bus, 100 * GST_MSECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_QOS |
                                                                             GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

        if (message)
        {
            if (GST_MESSAGE_SRC(message) == GST_OBJECT(m_context.pipeline))
            {
                switch (GST_MESSAGE_TYPE(message))
                {
                case GST_MESSAGE_STATE_CHANGED:
                {
                    GstState oldState, newState, pending;
                    m_gstWrapper->gstMessageParseStateChanged(message, &oldState, &newState, &pending);
                    switch (newState)
                    {
                    case GST_STATE_NULL:
                    {
                        m_isGstreamerDispatcherActive = false;
                    }
                    case GST_STATE_READY:
                    case GST_STATE_PAUSED:
                    case GST_STATE_PLAYING:
                    case GST_STATE_VOID_PENDING:
                    {
                        break;
                    }
                    }
                    break;
                }
                case GST_MESSAGE_ERROR:
                {
                    m_isGstreamerDispatcherActive = false;
                    break;
                }
                default:
                {
                    break;
                }
                }
            }

            m_workerThread.enqueueTask(m_kTaskFactory.createHandleBusMessage(m_context, m_player, message));
        }
    }

    RIALTO_SERVER_LOG_INFO("Gstbus dispatcher exitting");
    m_gstWrapper->gstObjectUnref(bus);
}
} // namespace firebolt::rialto::server
