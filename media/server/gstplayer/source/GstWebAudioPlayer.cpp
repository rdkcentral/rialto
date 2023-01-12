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

#include "GstWebAudioPlayer.h"
#include "GstDispatcherThread.h"
#include "RialtoServerLogging.h"
#include "WorkerThread.h"
#include "tasks/webAudio/WebAudioPlayerTaskFactory.h"

namespace firebolt::rialto::server
{
std::weak_ptr<IGstWebAudioPlayerFactory> GstWebAudioPlayerFactory::m_factory;

std::shared_ptr<IGstWebAudioPlayerFactory> IGstWebAudioPlayerFactory::getFactory()
{
    std::shared_ptr<IGstWebAudioPlayerFactory> factory = GstWebAudioPlayerFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<GstWebAudioPlayerFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer player factory, reason: %s", e.what());
        }

        GstWebAudioPlayerFactory::m_factory = factory;
    }

    return factory;
}

std::unique_ptr<IGstWebAudioPlayer> GstWebAudioPlayerFactory::createGstWebAudioPlayer(IGstWebAudioPlayerClient *client)
{
    std::unique_ptr<IGstWebAudioPlayer> gstPlayer;

    try
    {
        auto gstWrapperFactory = IGstWrapperFactory::getFactory();
        auto glibWrapperFactory = IGlibWrapperFactory::getFactory();
        std::shared_ptr<IGstWrapper> gstWrapper;
        std::shared_ptr<IGlibWrapper> glibWrapper;
        if ((!gstWrapperFactory) || (!(gstWrapper = gstWrapperFactory->getGstWrapper())))
        {
            throw std::runtime_error("Cannot create GstWrapper");
        }
        if ((!glibWrapperFactory) || (!(glibWrapper = glibWrapperFactory->getGlibWrapper())))
        {
            throw std::runtime_error("Cannot create GlibWrapper");
        }
        gstPlayer = std::make_unique<GstWebAudioPlayer>(client, gstWrapper, glibWrapper, IGstSrcFactory::getFactory(),
                                                        std::make_unique<WebAudioPlayerTaskFactory>(client, gstWrapper,
                                                                                                    glibWrapper),
                                                        std::make_unique<WorkerThreadFactory>(),
                                                        std::make_unique<GstDispatcherThreadFactory>());
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer player, reason: %s", e.what());
    }

    return gstPlayer;
}

GstWebAudioPlayer::GstWebAudioPlayer(IGstWebAudioPlayerClient *client, const std::shared_ptr<IGstWrapper> &gstWrapper,
                                     const std::shared_ptr<IGlibWrapper> &glibWrapper,
                                     const std::shared_ptr<IGstSrcFactory> &gstSrcFactory,
                                     std::unique_ptr<IWebAudioPlayerTaskFactory> taskFactory,
                                     std::unique_ptr<IWorkerThreadFactory> workerThreadFactory,
                                     std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory)
    : m_gstPlayerClient(client), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_taskFactory{
                                                                                           std::move(taskFactory)}
{
    RIALTO_SERVER_LOG_DEBUG("GstWebAudioPlayer is constructed.");

    if ((!gstSrcFactory) || (!(m_context.gstSrc = gstSrcFactory->getGstSrc())))
    {
        throw std::runtime_error("Cannot create GstSrc");
    }

    // Ensure that rialtosrc has been initalised
    m_context.gstSrc->initSrc();

    // Start task thread
    m_workerThread = workerThreadFactory->createWorkerThread();

    initWebAudioPipeline();

    m_gstDispatcherThread =
        gstDispatcherThreadFactory->createGstDispatcherThread(*this, m_context.pipeline, m_gstWrapper);
}

GstWebAudioPlayer::~GstWebAudioPlayer()
{
    RIALTO_SERVER_LOG_DEBUG("GstWebAudioPlayer is destructed.");

    m_gstDispatcherThread.reset();

    // TODO(RIALTO-2): Add shutdown task
    // Shutdown task thread
    m_workerThread->join();
    m_workerThread.reset();

    GstBus *bus = m_gstWrapper->gstPipelineGetBus(GST_PIPELINE(m_context.pipeline));
    m_gstWrapper->gstBusSetSyncHandler(bus, nullptr, nullptr, nullptr);
    m_gstWrapper->gstObjectUnref(bus);

    if (m_context.source)
    {
        m_gstWrapper->gstObjectUnref(m_context.source);
    }

    // Delete the pipeline
    m_glibWrapper->gObjectUnref(m_context.pipeline);
    m_glibWrapper->gObjectUnref(m_context.pipeline);
}

void GstWebAudioPlayer::initWebAudioPipeline() {}

void GstWebAudioPlayer::play() {}

void GstWebAudioPlayer::pause() {}

void GstWebAudioPlayer::setVolume(double volume) {}

bool GstWebAudioPlayer::getVolume(double &volume)
{
    return false;
}

void GstWebAudioPlayer::handleBusMessage(GstMessage *message) {}

bool GstWebAudioPlayer::changePipelineState(GstState newState)
{
    return false;
}
}; // namespace firebolt::rialto::server
