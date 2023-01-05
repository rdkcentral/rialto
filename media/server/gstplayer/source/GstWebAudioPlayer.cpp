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
        gstPlayer = std::make_unique<GstWebAudioPlayer>(client, gstWrapper,
                                                glibWrapper, IGstSrcFactory::getFactory(),
                                                std::make_unique<WebAudioPlayerTaskFactory>(client, gstWrapper, glibWrapper),
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
                     const std::shared_ptr<IGstSrcFactory> &gstSrcFactory, std::unique_ptr<IWebAudioPlayerTaskFactory> taskFactory,
                     std::unique_ptr<IWorkerThreadFactory> workerThreadFactory,
                     std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory)
    : m_gstPlayerClient(client), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_taskFactory{std::move(taskFactory)}
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

    m_gstDispatcherThread = gstDispatcherThreadFactory->createGstDispatcherThread(*this, m_context.pipeline, m_gstWrapper);
}

GstWebAudioPlayer::~GstWebAudioPlayer()
{
    RIALTO_SERVER_LOG_DEBUG("GstWebAudioPlayer is destructed.");

    m_gstDispatcherThread.reset();

    // Shutdown task thread
    //m_workerThread->enqueueTask(m_taskFactory->createShutdown(*this));
    m_workerThread->join();
    m_workerThread.reset();

    //m_taskFactory->createStop(m_context, *this)->execute();
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

void GstWebAudioPlayer::initWebAudioPipeline()
{
    m_context.pipeline = gst_pipeline_new("pipeline");

    // Create and initalise appsrc
    m_context.source = gst_element_factory_make("appsrc", "audsrc");

    gst_app_src_set_max_bytes(GST_APP_SRC(m_context.source), 10 * 1024);
    g_object_set(m_context.source, "format", GST_FORMAT_TIME, nullptr);

    // Perform sink specific initalisation
    GstRegistry *reg = gst_registry_get();
    GstElement *sink = nullptr;
    GstPluginFeature *feature = nullptr;
    if (nullptr != (feature = gst_registry_lookup_feature(reg, "amlhalasink")))
    {
        // LLama
        RIALTO_SERVER_LOG_INFO("Use amlhalasink");
        sink = gst_element_factory_make("amlhalasink", "uiaudiosink");
        g_object_set(G_OBJECT(sink), "direct-mode", FALSE, NULL);
        gst_bin_add_many(GST_BIN(m_context.pipeline), m_context.source, sink, NULL);
        gst_element_link_many(m_context.source, sink, NULL);
        gst_object_unref(feature);
    }
    else if (nullptr != (feature = gst_registry_lookup_feature(reg, "rtkaudiosink")))
    {
        // XiOne
        RIALTO_SERVER_LOG_INFO("Use rtkaudiosink");
        sink = gst_element_factory_make("rtkaudiosink", "uiaudiosink");
        g_object_set(G_OBJECT(sink), "media-tunnel", FALSE, NULL);
        g_object_set(G_OBJECT(sink), "audio-service", TRUE, NULL);
        GstElement *convert = NULL;
        GstElement *resample = NULL;
        //GstElement *volume = NULL;
        convert = gst_element_factory_make("audioconvert", NULL);
        resample = gst_element_factory_make("audioresample", NULL);
        //volume = gst_element_factory_make("volume", NULL);
        gst_bin_add_many(GST_BIN(m_context.pipeline), m_context.source, convert, resample, sink, nullptr);
        gst_element_link_many(m_context.source, convert, resample, sink, nullptr);
        gst_object_unref(feature);
    }
    else
    {
        RIALTO_SERVER_LOG_INFO("Use autoaudiosink");
        sink = gst_element_factory_make("autoaudiosink", "autoaudiosink");
        gst_bin_add_many(GST_BIN(m_context.pipeline), m_context.source, sink, NULL);
        gst_element_link_many(m_context.source, sink, NULL);
    }
}

void GstWebAudioPlayer::play()
{

}

void GstWebAudioPlayer::pause()
{

}

void GstWebAudioPlayer::setVolume(double volume)
{

}

bool GstWebAudioPlayer::getVolume(double &volume)
{
    return false;
}

void GstWebAudioPlayer::handleBusMessage(GstMessage *message)
{
    //m_workerThread->enqueueTask(m_taskFactory->createHandleBusMessage(m_context, *this, message));
}

bool GstWebAudioPlayer::changePipelineState(GstState newState)
{
#if 0
    if (!m_context.pipeline)
    {
        RIALTO_SERVER_LOG_ERROR("Change state failed - pipeline is nullptr");
        if (m_gstPlayerClient)
            m_gstPlayerClient->notifyPlaybackState(PlaybackState::FAILURE);
        return false;
    }
    if (m_gstWrapper->gstElementSetState(m_context.pipeline, newState) == GST_STATE_CHANGE_FAILURE)
    {
        RIALTO_SERVER_LOG_ERROR("Change state failed - Gstreamer returned an error");
        if (m_gstPlayerClient)
            m_gstPlayerClient->notifyPlaybackState(PlaybackState::FAILURE);
        return false;
    }
    return true;
#endif
    return false;
}
}; // namespace firebolt::rialto::server
