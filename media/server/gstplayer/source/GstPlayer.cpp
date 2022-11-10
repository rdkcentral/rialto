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

#include "GstPlayer.h"
#include "GstDispatcherThread.h"
#include "ITimer.h"
#include "RialtoServerLogging.h"
#include "WorkerThread.h"
#include "tasks/PlayerTaskFactory.h"
#include <IMediaPipeline.h>
#include <chrono>

namespace
{
/**
 * @brief The time to wait to finish the set up of sources.
 *        When the pipeline requests the set up of sources, GstPlayer must wait for all
 *        sources to be attached first.
 */
constexpr std::chrono::milliseconds kSourceSetupFinishTimeoutMs{200};

/**
 * @brief Report position interval in ms.
 *        The position reporting timer should be started whenever the PLAYING state is entered and stopped
 *        whenever the session moves to another playback state.
 */
constexpr std::chrono::milliseconds kPositionReportTimerMs{250};
} // namespace

namespace firebolt::rialto::server
{
bool m_gstInit = false;
std::weak_ptr<IGstPlayerFactory> GstPlayerFactory::m_factory;

std::shared_ptr<IGstPlayerFactory> IGstPlayerFactory::getFactory()
{
    std::shared_ptr<IGstPlayerFactory> factory = GstPlayerFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<GstPlayerFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer player factory, reason: %s", e.what());
        }

        GstPlayerFactory::m_factory = factory;
    }

    return factory;
}

std::unique_ptr<IGstPlayer> GstPlayerFactory::createGstPlayer(IGstPlayerClient *client,
                                                              IDecryptionService &decryptionService, MediaType type)
{
    std::unique_ptr<IGstPlayer> gstPlayer;

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
        gstPlayer = std::make_unique<GstPlayer>(client, decryptionService, type, gstWrapper, glibWrapper,
                                                IGstSrcFactory::getFactory(), common::ITimerFactory::getFactory(),
                                                std::make_unique<PlayerTaskFactory>(client, gstWrapper, glibWrapper),
                                                std::make_unique<WorkerThreadFactory>(),
                                                std::make_unique<GstDispatcherThreadFactory>());
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer player, reason: %s", e.what());
    }

    return gstPlayer;
}

bool IGstPlayer::initalise(int argc, char **argv)
{
    std::shared_ptr<IGstWrapperFactory> factory = IGstWrapperFactory::getFactory();
    std::shared_ptr<IGstWrapper> gstWrapper = factory->getGstWrapper();

    if (!gstWrapper)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gst wrapper");
        return false;
    }

    gstWrapper->gstInit(&argc, &argv);

    // remove rialto sinks from the registry
    GstPlugin *rialtoPlugin = gstWrapper->gstRegistryFindPlugin(gstWrapper->gstRegistryGet(), "rialtomsesinks");
    if (rialtoPlugin)
    {
        gstWrapper->gstRegistryRemovePlugin(gstWrapper->gstRegistryGet(), rialtoPlugin);
        gstWrapper->gstObjectUnref(rialtoPlugin);
    }

    return true;
}

GstPlayer::GstPlayer(IGstPlayerClient *client, IDecryptionService &decryptionService, MediaType type,
                     const std::shared_ptr<IGstWrapper> &gstWrapper, const std::shared_ptr<IGlibWrapper> &glibWrapper,
                     const std::shared_ptr<IGstSrcFactory> &gstSrcFactory,
                     std::shared_ptr<common::ITimerFactory> timerFactory, std::unique_ptr<IPlayerTaskFactory> taskFactory,
                     std::unique_ptr<IWorkerThreadFactory> workerThreadFactory,
                     std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory)
    : m_gstPlayerClient(client), m_decryptionService{decryptionService}, m_gstWrapper{gstWrapper},
      m_glibWrapper{glibWrapper}, m_timerFactory{timerFactory}, m_taskFactory{std::move(taskFactory)}
{
    RIALTO_SERVER_LOG_DEBUG("GstPlayer is constructed.");
    if ((!gstSrcFactory) || (!(m_context.gstSrc = gstSrcFactory->getGstSrc())))
    {
        throw std::runtime_error("Cannot create GstSrc");
    }
    if (!timerFactory)
    {
        throw std::runtime_error("TimeFactory is invalid");
    }

    // Ensure that rialtosrc has been initalised
    m_context.gstSrc->initSrc();

    // Start task thread
    m_workerThread = workerThreadFactory->createWorkerThread();

    // Initialise pipeline
    switch (type)
    {
    case MediaType::MSE:
    {
        initMsePipeline();
        break;
    }
    default:
    {
        throw std::runtime_error("Media type not supported");
    }
    }

    m_gstDispatcherThread = gstDispatcherThreadFactory->createGstDispatcherThread(m_context, *this, m_gstWrapper,
                                                                                  *m_workerThread, *m_taskFactory);
}

GstPlayer::~GstPlayer()
{
    RIALTO_SERVER_LOG_DEBUG("GstPlayer is destructed.");

    m_gstDispatcherThread.reset();

    // Shutdown task thread
    m_workerThread->enqueueTask(m_taskFactory->createShutdown(*this));
    m_workerThread->join();
    m_workerThread.reset();

    if (m_finishSourceSetupTimer && m_finishSourceSetupTimer->isActive())
    {
        m_finishSourceSetupTimer->cancel();
    }

    m_finishSourceSetupTimer.reset();

    for (auto &buffer : m_context.audioBuffers)
    {
        m_gstWrapper->gstBufferUnref(buffer);
    }
    m_context.audioBuffers.clear();
    for (auto &buffer : m_context.videoBuffers)
    {
        m_gstWrapper->gstBufferUnref(buffer);
    }
    m_context.videoBuffers.clear();

    m_taskFactory->createStop(m_context, *this)->execute();
    GstBus *bus = m_gstWrapper->gstPipelineGetBus(GST_PIPELINE(m_context.pipeline));
    m_gstWrapper->gstBusSetSyncHandler(bus, nullptr, nullptr, nullptr);
    m_gstWrapper->gstObjectUnref(bus);

    if (m_context.source)
    {
        m_gstWrapper->gstObjectUnref(m_context.source);
    }

    // Delete the pipeline
    m_glibWrapper->gObjectUnref(m_context.pipeline);
}

void GstPlayer::initMsePipeline()
{
    // Make playbin
    m_context.pipeline = m_gstWrapper->gstElementFactoryMake("playbin", "media_pipeline");
    // Set pipeline flags
    unsigned flagAudio = getGstPlayFlag("audio");
    unsigned flagVideo = getGstPlayFlag("video");
    unsigned flagNativeVideo = getGstPlayFlag("native-video");
    unsigned flagNativeAudio = 0;
    m_glibWrapper->gObjectSet(m_context.pipeline, "flags", flagAudio | flagVideo | flagNativeVideo | flagNativeAudio,
                              nullptr);

    // Set callbacks
    m_glibWrapper->gSignalConnect(m_context.pipeline, "source-setup", G_CALLBACK(&GstPlayer::setupSource), this);
    m_glibWrapper->gSignalConnect(m_context.pipeline, "element-setup", G_CALLBACK(&GstPlayer::setupElement), this);

    // Set uri
    m_glibWrapper->gObjectSet(m_context.pipeline, "uri", "rialto://", nullptr);

    // Check playsink
    GstElement *playsink = (m_gstWrapper->gstBinGetByName(GST_BIN(m_context.pipeline), "playsink"));
    if (playsink)
    {
        m_glibWrapper->gObjectSet(G_OBJECT(playsink), "send-event-mode", 0, nullptr);
        m_glibWrapper->gObjectUnref(playsink);
    }
    else
    {
        GST_WARNING("No playsink ?!?!?");
    }
}

unsigned GstPlayer::getGstPlayFlag(const char *nick)
{
    GFlagsClass *flagsClass =
        static_cast<GFlagsClass *>(m_glibWrapper->gTypeClassRef(m_glibWrapper->gTypeFromName("GstPlayFlags")));
    GFlagsValue *flag = m_glibWrapper->gFlagsGetValueByNick(flagsClass, nick);
    return flag ? flag->value : 0;
}

void GstPlayer::setupSource(GstElement *pipeline, GstElement *source, GstPlayer *self)
{
    self->m_gstWrapper->gstObjectRef(source);
    if (self->m_workerThread)
    {
        self->m_workerThread->enqueueTask(self->m_taskFactory->createSetupSource(self->m_context, *self, source));
    }
}

void GstPlayer::scheduleSourceSetupFinish()
{
    m_finishSourceSetupTimer =
        m_timerFactory->createTimer(kSourceSetupFinishTimeoutMs,
                                    [this]()
                                    {
                                        if (m_workerThread)
                                        {
                                            m_workerThread->enqueueTask(
                                                m_taskFactory->createFinishSetupSource(m_context, *this));
                                        }
                                    });
}

void GstPlayer::setupElement(GstElement *pipeline, GstElement *element, GstPlayer *self)
{
    RIALTO_SERVER_LOG_DEBUG("Element %s added to the pipeline", GST_ELEMENT_NAME(element));
    self->m_gstWrapper->gstObjectRef(element);
    if (self->m_workerThread)
    {
        self->m_workerThread->enqueueTask(self->m_taskFactory->createSetupElement(self->m_context, *self, element));
    }
}

void GstPlayer::attachSource(const IMediaPipeline::MediaSource &attachedSource)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createAttachSource(m_context, attachedSource));
    }
}

void GstPlayer::attachSamples(const IMediaPipeline::MediaSegmentVector &mediaSegments)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createAttachSamples(m_context, *this, mediaSegments));
    }
}

void GstPlayer::attachSamples(const std::shared_ptr<IDataReader> &dataReader)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createReadShmDataAndAttachSamples(m_context, *this, dataReader));
    }
}

void GstPlayer::setPosition(std::int64_t position)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetPosition(m_context, *this, position));
    }
}

void GstPlayer::setPlaybackRate(double rate)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetPlaybackRate(m_context, rate));
    }
}

bool GstPlayer::getPosition(std::int64_t &position)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstPlayer
    // constructor and destructor. GstPlayer is created/destructed on main thread, so we won't have a crash here.
    if (!m_context.pipeline || GST_STATE(m_context.pipeline) < GST_STATE_PLAYING)
    {
        return false;
    }
    if (!m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &position))
    {
        return false;
    }
    return true;
}

GstBuffer *GstPlayer::createDecryptedBuffer(const IMediaPipeline::MediaSegment &mediaSegment) const
{
    GstBuffer *gstBuffer = m_gstWrapper->gstBufferNewAllocate(nullptr, mediaSegment.getDataLength(), nullptr);
    m_gstWrapper->gstBufferFill(gstBuffer, 0, mediaSegment.getData(), mediaSegment.getDataLength());

    if (mediaSegment.isEncrypted())
    {
        GstBuffer *keyId = m_gstWrapper->gstBufferNewAllocate(nullptr, mediaSegment.getKeyId().size(), nullptr);
        m_gstWrapper->gstBufferFill(keyId, 0, mediaSegment.getKeyId().data(), mediaSegment.getKeyId().size());
        GstBuffer *initVector = m_gstWrapper->gstBufferNewAllocate(nullptr, mediaSegment.getInitVector().size(), nullptr);
        m_gstWrapper->gstBufferFill(initVector, 0, mediaSegment.getInitVector().data(),
                                    mediaSegment.getInitVector().size());
        auto subsamplesRawSize = mediaSegment.getSubSamples().size() * (sizeof(guint16) + sizeof(guint32));
        guint8 *subsamplesRaw = static_cast<guint8 *>(m_glibWrapper->gMalloc(subsamplesRawSize));
        GstByteWriter writer;
        m_gstWrapper->gstByteWriterInitWithData(&writer, subsamplesRaw, subsamplesRawSize, FALSE);

        for (const auto &subSample : mediaSegment.getSubSamples())
        {
            m_gstWrapper->gstByteWriterPutUint16Be(&writer, subSample.numClearBytes);
            m_gstWrapper->gstByteWriterPutUint32Be(&writer, subSample.numEncryptedBytes);
        }
        GstBuffer *subsamples = m_gstWrapper->gstBufferNewWrapped(subsamplesRaw, subsamplesRawSize);

        m_decryptionService.decrypt(mediaSegment.getMediaKeySessionId(), gstBuffer, subsamples,
                                    mediaSegment.getSubSamples().size(), initVector, keyId,
                                    mediaSegment.getInitWithLast15());
    }

    GST_BUFFER_TIMESTAMP(gstBuffer) = mediaSegment.getTimeStamp();
    GST_BUFFER_DURATION(gstBuffer) = mediaSegment.getDuration();
    return gstBuffer;
}

void GstPlayer::notifyNeedMediaData(bool audioNotificationNeeded, bool videoNotificationNeeded)
{
    if (audioNotificationNeeded)
    {
        // Mark needMediaData as received
        m_context.audioNeedDataPending = false;
        // Send new NeedMediaData if we still need it
        if (m_gstPlayerClient && m_context.audioNeedData)
        {
            m_context.audioNeedDataPending = m_gstPlayerClient->notifyNeedMediaData(MediaSourceType::AUDIO);
        }
    }
    else if (videoNotificationNeeded)
    {
        // Mark needMediaData as received
        m_context.videoNeedDataPending = false;
        // Send new NeedMediaData if we still need it
        if (m_gstPlayerClient && m_context.videoNeedData)
        {
            m_context.videoNeedDataPending = m_gstPlayerClient->notifyNeedMediaData(MediaSourceType::VIDEO);
        }
    }
}

void GstPlayer::attachAudioData()
{
    if (m_context.audioBuffers.empty() || !m_context.audioNeedData)
    {
        return;
    }
    if (!m_context.audioAppSrc)
    {
        auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO);
        if (elem != m_context.streamInfo.end())
        {
            m_context.audioAppSrc = elem->second;
        }
    }

    if (m_context.audioAppSrc)
    {
        for (GstBuffer *buffer : m_context.audioBuffers)
        {
            m_gstWrapper->gstAppSrcPushBuffer(GST_APP_SRC(m_context.audioAppSrc), buffer);
        }

        if (m_context.audioBuffers.size())
        {
            m_context.lastAudioSampleTimestamps = static_cast<int64_t>(GST_BUFFER_PTS(m_context.audioBuffers.back()));
        }
        m_context.audioBuffers.clear();
        m_context.audioDataPushed = true;
        if (!m_context.bufferedNotificationSent && m_context.videoDataPushed && m_gstPlayerClient)
        {
            m_context.bufferedNotificationSent = true;
            m_gstPlayerClient->notifyNetworkState(NetworkState::BUFFERED);
        }
        cancelUnderflow(m_context.audioUnderflowOccured);
    }
}

void GstPlayer::attachVideoData()
{
    if (m_context.videoBuffers.empty() || !m_context.videoNeedData)
    {
        return;
    }
    if (!m_context.videoAppSrc)
    {
        auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO);
        if (elem != m_context.streamInfo.end())
        {
            m_context.videoAppSrc = elem->second;
        }
    }
    if (m_context.videoAppSrc)
    {
        for (GstBuffer *buffer : m_context.videoBuffers)
        {
            m_gstWrapper->gstAppSrcPushBuffer(GST_APP_SRC(m_context.videoAppSrc), buffer);
        }
        m_context.videoBuffers.clear();
        m_context.videoDataPushed = true;
        if (!m_context.bufferedNotificationSent && m_context.audioDataPushed && m_gstPlayerClient)
        {
            m_context.bufferedNotificationSent = true;
            m_gstPlayerClient->notifyNetworkState(NetworkState::BUFFERED);
        }
        cancelUnderflow(m_context.videoUnderflowOccured);
    }
}

void GstPlayer::updateAudioCaps(int32_t rate, int32_t channels)
{
    if (!m_context.audioAppSrc)
    {
        auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO);
        if (elem != m_context.streamInfo.end())
        {
            m_context.audioAppSrc = elem->second;
        }
    }

    if (m_context.audioAppSrc)
    {
        constexpr int kInvalidRate{0}, kInvalidChannels{0};
        bool capsChanged{false};
        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(m_context.audioAppSrc));
        GstCaps *newCaps = m_gstWrapper->gstCapsCopy(currentCaps);
        if (rate != kInvalidRate)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "rate", G_TYPE_INT, rate, NULL);
            capsChanged = true;
        }
        if (channels != kInvalidChannels)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "channels", G_TYPE_INT, channels, NULL);
            capsChanged = true;
        }
        if (capsChanged)
        {
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(m_context.audioAppSrc), newCaps);
        }
        m_gstWrapper->gstCapsUnref(newCaps);
        m_gstWrapper->gstCapsUnref(currentCaps);
    }
}

void GstPlayer::updateVideoCaps(int32_t width, int32_t height)
{
    if (!m_context.videoAppSrc)
    {
        auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO);
        if (elem != m_context.streamInfo.end())
        {
            m_context.videoAppSrc = elem->second;
        }
    }

    if (m_context.videoAppSrc)
    {
        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(m_context.videoAppSrc));
        GstCaps *newCaps = m_gstWrapper->gstCapsCopy(currentCaps);

        m_gstWrapper->gstCapsSetSimple(newCaps, "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);

        m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(m_context.videoAppSrc), newCaps);

        m_gstWrapper->gstCapsUnref(currentCaps);
        m_gstWrapper->gstCapsUnref(newCaps);
    }
}

void GstPlayer::scheduleNeedMediaData(GstAppSrc *src)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createNeedData(m_context, src));
    }
}

void GstPlayer::scheduleEnoughData(GstAppSrc *src)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createEnoughData(m_context, src));
    }
}

void GstPlayer::scheduleAudioUnderflow()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createUnderflow(*this, m_context.audioUnderflowOccured));
    }
}

void GstPlayer::scheduleVideoUnderflow()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createUnderflow(*this, m_context.videoUnderflowOccured));
    }
}

void GstPlayer::cancelUnderflow(bool &underflowFlag)
{
    if (!underflowFlag)
    {
        return;
    }
    underflowFlag = false;
    if (!m_context.audioUnderflowOccured && !m_context.videoUnderflowOccured)
    {
        m_taskFactory->createPlay(*this)->execute();
        m_gstPlayerClient->notifyNetworkState(NetworkState::BUFFERED);
    }
}

void GstPlayer::play()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createPlay(*this));
    }
}

void GstPlayer::pause()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createPause(*this));
    }
}

void GstPlayer::stop()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createStop(m_context, *this));
    }
}

bool GstPlayer::changePipelineState(GstState newState)
{
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
}

void GstPlayer::setVideoGeometry(int x, int y, int width, int height)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createSetVideoGeometry(m_context, *this, Rectangle{x, y, width, height}));
    }
}

void GstPlayer::setEos(const firebolt::rialto::MediaSourceType &type)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createEos(m_context, *this, type));
    }
}

bool GstPlayer::setWesterossinkRectangle()
{
    bool result = false;
    GstElement *videoSink = nullptr;
    m_glibWrapper->gObjectGet(m_context.pipeline, "video-sink", &videoSink, nullptr);
    if (videoSink && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), "rectangle"))
    {
        char rect[64];
        snprintf(rect, sizeof(rect), "%d,%d,%d,%d", m_context.pendingGeometry.x, m_context.pendingGeometry.y,
                 m_context.pendingGeometry.width, m_context.pendingGeometry.height);
        m_glibWrapper->gObjectSet(videoSink, "rectangle", rect, nullptr);
        m_context.pendingGeometry.clear();
        result = true;
    }

    if (videoSink)
        m_gstWrapper->gstObjectUnref(GST_OBJECT(videoSink));

    return result;
}

void GstPlayer::startPositionReportingAndCheckAudioUnderflowTimer()
{
    if (m_positionReportingAndCheckAudioUnderflowTimer && m_positionReportingAndCheckAudioUnderflowTimer->isActive())
    {
        return;
    }

    m_positionReportingAndCheckAudioUnderflowTimer = m_timerFactory->createTimer(
        kPositionReportTimerMs,
        [this]()
        {
            if (m_workerThread)
            {
                m_workerThread->enqueueTask(m_taskFactory->createReportPosition(m_context));
                m_workerThread->enqueueTask(m_taskFactory->createCheckAudioUnderflow(m_context, *this));
            }
        },
        firebolt::rialto::common::TimerType::PERIODIC);
}

void GstPlayer::stopPositionReportingAndCheckAudioUnderflowTimer()
{
    if (m_positionReportingAndCheckAudioUnderflowTimer && m_positionReportingAndCheckAudioUnderflowTimer->isActive())
    {
        m_positionReportingAndCheckAudioUnderflowTimer->cancel();
        m_positionReportingAndCheckAudioUnderflowTimer.reset();
    }
}

void GstPlayer::stopWorkerThread()
{
    if (m_workerThread)
    {
        m_workerThread->stop();
    }
}

void GstPlayer::setPendingPlaybackRate()
{
    RIALTO_SERVER_LOG_INFO("Setting pending playback rate");
    setPlaybackRate(m_context.pendingPlaybackRate);
}
}; // namespace firebolt::rialto::server
