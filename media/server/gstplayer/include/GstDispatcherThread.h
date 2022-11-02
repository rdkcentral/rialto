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
    createGstDispatcherThread(PlayerContext &playerContext, IGstPlayerPrivate &player,
                              const std::shared_ptr<IGstWrapper> &gstWrapper, IWorkerThread &workerThread,
                              const IPlayerTaskFactory &taskFactory) const override;
};

class GstDispatcherThread : public IGstDispatcherThread
{
public:
    GstDispatcherThread(PlayerContext &playerContext, IGstPlayerPrivate &player,
                        const std::shared_ptr<IGstWrapper> &gstWrapper, IWorkerThread &workerThread,
                        const IPlayerTaskFactory &taskFactory);
    ~GstDispatcherThread() override;

private:
    /**
     * @brief For handling gst bus messages in Gstreamer dispatcher thread
     * @param[in] pipeline : The pipeline
     */
    void gstBusEventHandler(GstElement *pipeline);

private:
    /**
     * @brief The player context.
     */
    PlayerContext &m_context;

    /**
     * @brief The gst player instance.
     */
    IGstPlayerPrivate &m_player;

    /**
     * @brief The gstreamer wrapper object.
     */
    std::shared_ptr<IGstWrapper> m_gstWrapper;

    /**
     * @brief Thread for handling player tasks.
     */
    IWorkerThread &m_workerThread;

    /**
     * @brief The GstPlayer task factory
     */
    const IPlayerTaskFactory &m_kTaskFactory;

    /**
     * @brief Flag used to check, if task thread is active
     */
    std::atomic<bool> m_isGstreamerDispatcherActive;

    /**
     * @brief Thread for handling gst bus callbacks
     */
    std::thread m_gstBusDispatcherThread;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_DISPATCHER_THREAD_H_
