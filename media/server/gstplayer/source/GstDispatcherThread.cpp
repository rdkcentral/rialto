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
#include "RialtoServerLogging.h"

namespace
{
// Messages that were previously requested via gst_bus_timed_pop_filtered's type mask.
constexpr GstMessageType kHandledMessageTypes =
    static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_QOS | GST_MESSAGE_EOS | GST_MESSAGE_ERROR |
                                GST_MESSAGE_WARNING | GST_MESSAGE_APPLICATION);
} // namespace

namespace firebolt::rialto::server
{
std::unique_ptr<IGstDispatcherThread> GstDispatcherThreadFactory::createGstDispatcherThread(
    IGstDispatcherThreadClient &client, GstElement *pipeline,
    const std::shared_ptr<IFlushOnPrerollController> &flushOnPrerollController,
    const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper) const
{
    return std::make_unique<GstDispatcherThread>(client, pipeline, flushOnPrerollController, gstWrapper, glibWrapper);
}

GstDispatcherThread::GstDispatcherThread(IGstDispatcherThreadClient &client, GstElement *pipeline,
                                         const std::shared_ptr<IFlushOnPrerollController> &flushOnPrerollController,
                                         const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                         const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper)
    : m_client{client}, m_flushOnPrerollController{flushOnPrerollController}, m_gstWrapper{gstWrapper},
      m_glibWrapper{glibWrapper}, m_isGstreamerDispatcherActive{true}, m_pipeline{pipeline}
{
    RIALTO_SERVER_LOG_INFO("GstDispatcherThread is starting");

    GstBus *bus = m_gstWrapper->gstPipelineGetBus(GST_PIPELINE(pipeline));
    if (!bus)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get gst bus");
        return;
    }

    // Own GMainContext so this dispatcher doesn't share a thread/wheel with other GLib users in the process.
    m_mainContext = m_glibWrapper->gMainContextNew();
    m_mainLoop = m_glibWrapper->gMainLoopNew(m_mainContext, FALSE);

    GSource *busSource = m_gstWrapper->gstBusCreateWatch(bus);
    m_glibWrapper->gSourceSetCallback(busSource, reinterpret_cast<GSourceFunc>(&GstDispatcherThread::onBusMessage),
                                      this, nullptr);
    m_glibWrapper->gSourceAttach(busSource, m_mainContext);
    m_glibWrapper->gSourceUnref(busSource);
    m_gstWrapper->gstObjectUnref(bus);

    m_gstBusDispatcherThread = std::thread(&GstDispatcherThread::gstBusEventHandler, this);
}

GstDispatcherThread::~GstDispatcherThread()
{
    RIALTO_SERVER_LOG_INFO("Stopping GstDispatcherThread");
    m_isGstreamerDispatcherActive = false;
    if (m_mainLoop)
    {
        m_glibWrapper->gMainLoopQuit(m_mainLoop);
    }
    if (m_gstBusDispatcherThread.joinable())
    {
        m_gstBusDispatcherThread.join();
    }
    if (m_mainLoop)
    {
        m_glibWrapper->gMainLoopUnref(m_mainLoop);
        m_glibWrapper->gMainContextUnref(m_mainContext);
    }
}

gboolean GstDispatcherThread::onBusMessage(GstBus * /*bus*/, GstMessage *message, gpointer userData)
{
    static_cast<GstDispatcherThread *>(userData)->handleMessage(message);
    return G_SOURCE_CONTINUE;
}

void GstDispatcherThread::gstBusEventHandler()
{
    m_glibWrapper->gMainContextPushThreadDefault(m_mainContext);
    m_glibWrapper->gMainLoopRun(m_mainLoop);
    m_glibWrapper->gMainContextPopThreadDefault(m_mainContext);

    RIALTO_SERVER_LOG_INFO("Gstbus dispatcher exitting");
}

void GstDispatcherThread::handleMessage(GstMessage *message)
{
    // The bus watch delivers every message; only these types were previously popped from the bus at all.
    if (!(GST_MESSAGE_TYPE(message) & kHandledMessageTypes))
    {
        return;
    }

    bool shouldHandleMessage{true};
    if (GST_MESSAGE_SRC(message) == GST_OBJECT(m_pipeline))
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
                if (m_flushOnPrerollController)
                {
                    m_flushOnPrerollController->reset();
                }
                break;
            }
            case GST_STATE_PAUSED:
            {
                if (m_flushOnPrerollController && pending != GST_STATE_PAUSED)
                {
                    m_flushOnPrerollController->stateReached(newState);
                }
                else if (m_flushOnPrerollController && pending == GST_STATE_PAUSED)
                {
                    m_flushOnPrerollController->setPrerolling();
                }
                break;
            }
            case GST_STATE_PLAYING:
            {
                if (m_flushOnPrerollController)
                {
                    m_flushOnPrerollController->stateReached(newState);
                }
                break;
            }
            case GST_STATE_READY:
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
    else if (GST_MESSAGE_STATE_CHANGED == GST_MESSAGE_TYPE(message))
    {
        // Skip handling GST_MESSAGE_STATE_CHANGED for non-pipeline objects.
        // It signifficantly slows down rialto gst worker thread
        shouldHandleMessage = false;
    }

    if (shouldHandleMessage)
    {
        // The bus watch source unrefs message once this callback returns; the async task needs its own ref.
        m_client.handleBusMessage(m_gstWrapper->gstMessageRef(message));
    }

    if (!m_isGstreamerDispatcherActive)
    {
        m_glibWrapper->gMainLoopQuit(m_mainLoop);
    }
}
} // namespace firebolt::rialto::server

