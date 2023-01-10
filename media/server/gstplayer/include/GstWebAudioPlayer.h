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

#ifndef FIREBOLT_RIALTO_SERVER_GST_WEB_AUDIO_PLAYER_H_
#define FIREBOLT_RIALTO_SERVER_GST_WEB_AUDIO_PLAYER_H_

#include "IGlibWrapper.h"
#include "IGstDispatcherThread.h"
#include "IGstWebAudioPlayer.h"
#include "IGstWebAudioPlayerPrivate.h"
#include "IGstSrc.h"
#include "IGstWrapper.h"
#include "IWorkerThread.h"
#include "WebAudioPlayerContext.h"
#include "tasks/IPlayerTask.h"
#include "tasks/IWebAudioPlayerTaskFactory.h"
#include "IGstDispatcherThreadClient.h"
#include <memory>
#include <string>

namespace firebolt::rialto::server
{
/**
 * @brief IGstWebAudioPlayer factory class definition.
 */
class GstWebAudioPlayerFactory : public IGstWebAudioPlayerFactory
{
public:
    /**
     * @brief Weak pointer to the singleton factory object.
     */
    static std::weak_ptr<IGstWebAudioPlayerFactory> m_factory;

    std::unique_ptr<IGstWebAudioPlayer> createGstWebAudioPlayer(IGstWebAudioPlayerClient *client) override;
};

/**
 * @brief The definition of the GstWebAudioPlayer.
 */
class GstWebAudioPlayer : public IGstWebAudioPlayer, public IGstWebAudioPlayerPrivate, public IGstDispatcherThreadClient
{
public:
    /**
     * @brief The constructor.
     *
     * @param[in] client                       : The gstreamer player client.
     * @param[in] gstWrapper                   : The gstreamer wrapper.
     * @param[in] glibWrapper                  : The glib wrapper.
     * @param[in] gstSrcFactory                : The gstreamer rialto src factory.
     * @param[in] taskFactory                  : The task factory
     * @param[in] workerThreadFactory          : The worker thread factory
     * @param[in] gstDispatcherThreadFactory   : The gst dispatcher thread factory
     */
    GstWebAudioPlayer(IGstWebAudioPlayerClient *client, const std::shared_ptr<IGstWrapper> &gstWrapper,
              const std::shared_ptr<IGlibWrapper> &glibWrapper, const std::shared_ptr<IGstSrcFactory> &gstSrcFactory, std::unique_ptr<IWebAudioPlayerTaskFactory> taskFactory,
              std::unique_ptr<IWorkerThreadFactory> workerThreadFactory,
              std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory);

    /**
     * @brief Virtual destructor.
     */
    virtual ~GstWebAudioPlayer();

    void play() override;
    void pause() override;
    void setVolume(double volume) override;
    bool getVolume(double &volume) override;

    bool changePipelineState(GstState newState) override;
    void handleBusMessage(GstMessage *message) override;

private:
    /**
     * @brief Initialises the player pipeline for MSE playback.
     */
    void initWebAudioPipeline();

private:
    /**
     * @brief The player context.
     */
    WebAudioPlayerContext m_context;

    /**
     * @brief The gstreamer player client.
     */
    IGstWebAudioPlayerClient *m_gstPlayerClient{nullptr};

    /**
     * @brief The gstreamer wrapper object.
     */
    std::shared_ptr<IGstWrapper> m_gstWrapper;

    /**
     * @brief The glib wrapper object.
     */
    std::shared_ptr<IGlibWrapper> m_glibWrapper;

    /**
     * @brief Thread for handling player tasks.
     */
    std::unique_ptr<IWorkerThread> m_workerThread;

    /**
     * @brief Thread for handling gst bus callbacks
     */
    std::unique_ptr<IGstDispatcherThread> m_gstDispatcherThread;

    /**
     * @brief The GstWebAudioPlayer task factory
     */
    std::unique_ptr<IWebAudioPlayerTaskFactory> m_taskFactory;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_WEB_AUDIO_PLAYER_H_
