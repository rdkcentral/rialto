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

#ifndef FIREBOLT_RIALTO_SERVER_GST_DISPATCHER_THREAD_H_
#define FIREBOLT_RIALTO_SERVER_GST_DISPATCHER_THREAD_H_

#include "IGlibWrapper.h"
#include "IGstDispatcherThread.h"
#include <atomic>
#include <gst/gst.h>
#include <memory>
#include <thread>

namespace firebolt::rialto::server
{
class GstDispatcherThreadFactory : public IGstDispatcherThreadFactory
{
public:
    ~GstDispatcherThreadFactory() override = default;
    std::unique_ptr<IGstDispatcherThread>
    createGstDispatcherThread(IGstDispatcherThreadClient &client, GstElement *pipeline,
                              const std::shared_ptr<IFlushOnPrerollController> &flushOnPrerollController,
                              const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                              const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper) const override;
};

class GstDispatcherThread : public IGstDispatcherThread
{
public:
    GstDispatcherThread(IGstDispatcherThreadClient &client, GstElement *pipeline,
                        const std::shared_ptr<IFlushOnPrerollController> &flushOnPrerollController,
                        const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                        const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper);
    ~GstDispatcherThread() override;

private:
    /**
     * @brief Runs the GMainLoop that drives the bus watch. Runs on m_gstBusDispatcherThread.
     */
    void gstBusEventHandler();

    /**
     * @brief For handling gst bus messages, invoked by the bus watch on m_gstBusDispatcherThread.
     *
     * @param[in] message : The message popped from the bus.
     */
    void handleMessage(GstMessage *message);

    /**
     * @brief GSourceFunc-compatible trampoline registered as the bus watch's callback.
     */
    static gboolean onBusMessage(GstBus *bus, GstMessage *message, gpointer userData);

private:
    /**
     * @brief The listening client.
     */
    IGstDispatcherThreadClient &m_client;

    /**
     * @brief The flush on preroll controller.
     */
    std::shared_ptr<IFlushOnPrerollController> m_flushOnPrerollController;

    /**
     * @brief The gstreamer wrapper object.
     */
    std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> m_gstWrapper;

    /**
     * @brief The glib wrapper object.
     */
    std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> m_glibWrapper;

    /**
     * @brief Flag used to check, if the dispatcher should keep running.
     */
    std::atomic<bool> m_isGstreamerDispatcherActive;

    /**
     * @brief The pipeline being watched, used to identify pipeline-scoped messages.
     */
    GstElement *m_pipeline;

    /**
     * @brief Thread for handling gst bus callbacks
     */
    std::thread m_gstBusDispatcherThread;

    /**
     * @brief Private main context the bus watch is attached to. Only touched on the constructing thread
     *        and m_gstBusDispatcherThread; null if construction failed to obtain a bus.
     */
    GMainContext *m_mainContext{nullptr};

    /**
     * @brief The main loop that drives the bus watch.
     */
    GMainLoop *m_mainLoop{nullptr};
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_DISPATCHER_THREAD_H_

