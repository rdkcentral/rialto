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

#include <chrono>
#include <cinttypes>
#include <stdexcept>

#include "GstDispatcherThread.h"
#include "GstGenericPlayer.h"
#include "GstProtectionMetadata.h"
#include "IMediaPipeline.h"
#include "ITimer.h"
#include "RialtoServerLogging.h"
#include "WorkerThread.h"
#include "tasks/generic/GenericPlayerTaskFactory.h"

namespace
{
/**
 * @brief Report position interval in ms.
 *        The position reporting timer should be started whenever the PLAYING state is entered and stopped
 *        whenever the session moves to another playback state.
 */
constexpr std::chrono::milliseconds kPositionReportTimerMs{250};
} // namespace

namespace firebolt::rialto::server
{
std::weak_ptr<IGstGenericPlayerFactory> GstGenericPlayerFactory::m_factory;

std::shared_ptr<IGstGenericPlayerFactory> IGstGenericPlayerFactory::getFactory()
{
    std::shared_ptr<IGstGenericPlayerFactory> factory = GstGenericPlayerFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<GstGenericPlayerFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer player factory, reason: %s", e.what());
        }

        GstGenericPlayerFactory::m_factory = factory;
    }

    return factory;
}

std::unique_ptr<IGstGenericPlayer> GstGenericPlayerFactory::createGstGenericPlayer(
    IGstGenericPlayerClient *client, IDecryptionService &decryptionService, MediaType type,
    const VideoRequirements &videoRequirements,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapperFactory> &rdkGstreamerUtilsWrapperFactory)
{
    std::unique_ptr<IGstGenericPlayer> gstPlayer;

    try
    {
        auto gstWrapperFactory = firebolt::rialto::wrappers::IGstWrapperFactory::getFactory();
        auto glibWrapperFactory = firebolt::rialto::wrappers::IGlibWrapperFactory::getFactory();
        std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper;
        std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper;
        std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper;
        if ((!gstWrapperFactory) || (!(gstWrapper = gstWrapperFactory->getGstWrapper())))
        {
            throw std::runtime_error("Cannot create GstWrapper");
        }
        if ((!glibWrapperFactory) || (!(glibWrapper = glibWrapperFactory->getGlibWrapper())))
        {
            throw std::runtime_error("Cannot create GlibWrapper");
        }
        if ((!rdkGstreamerUtilsWrapperFactory) ||
            (!(rdkGstreamerUtilsWrapper = rdkGstreamerUtilsWrapperFactory->createRdkGstreamerUtilsWrapper())))
        {
            throw std::runtime_error("Cannot create RdkGstreamerUtilsWrapper");
        }
        gstPlayer =
            std::make_unique<GstGenericPlayer>(client, decryptionService, type, videoRequirements, gstWrapper,
                                               glibWrapper, IGstSrcFactory::getFactory(),
                                               common::ITimerFactory::getFactory(),
                                               std::make_unique<GenericPlayerTaskFactory>(client, gstWrapper, glibWrapper,
                                                                                          rdkGstreamerUtilsWrapper),
                                               std::make_unique<WorkerThreadFactory>(),
                                               std::make_unique<GstDispatcherThreadFactory>(),
                                               IGstProtectionMetadataHelperFactory::createFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer player, reason: %s", e.what());
    }

    return gstPlayer;
}

GstGenericPlayer::GstGenericPlayer(IGstGenericPlayerClient *client, IDecryptionService &decryptionService,
                                   MediaType type, const VideoRequirements &videoRequirements,
                                   const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                   const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                                   const std::shared_ptr<IGstSrcFactory> &gstSrcFactory,
                                   std::shared_ptr<common::ITimerFactory> timerFactory,
                                   std::unique_ptr<IGenericPlayerTaskFactory> taskFactory,
                                   std::unique_ptr<IWorkerThreadFactory> workerThreadFactory,
                                   std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory,
                                   std::shared_ptr<IGstProtectionMetadataHelperFactory> gstProtectionMetadataFactory)
    : m_gstPlayerClient(client), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_timerFactory{timerFactory},
      m_taskFactory{std::move(taskFactory)}
{
    RIALTO_SERVER_LOG_DEBUG("GstGenericPlayer is constructed.");

    m_context.decryptionService = &decryptionService;

    if ((!gstSrcFactory) || (!(m_context.gstSrc = gstSrcFactory->getGstSrc())))
    {
        throw std::runtime_error("Cannot create GstSrc");
    }

    if (!timerFactory)
    {
        throw std::runtime_error("TimeFactory is invalid");
    }

    if ((!gstProtectionMetadataFactory) ||
        (!(m_protectionMetadataWrapper = gstProtectionMetadataFactory->createProtectionMetadataWrapper(m_gstWrapper))))
    {
        throw std::runtime_error("Cannot create protection metadata wrapper");
    }

    // Ensure that rialtosrc has been initalised
    m_context.gstSrc->initSrc();

    // Start task thread
    if ((!workerThreadFactory) || (!(m_workerThread = workerThreadFactory->createWorkerThread())))
    {
        throw std::runtime_error("Failed to create the worker thread");
    }

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
        resetWorkerThread();
        throw std::runtime_error("Media type not supported");
    }
    }

    // Check the video requirements for a limited video.
    // If the video requirements are set to anything lower than the minimum, this playback is assumed to be a secondary
    // video in a dual video scenario.
    if ((kMinPrimaryVideoWidth > videoRequirements.maxWidth) || (kMinPrimaryVideoHeight > videoRequirements.maxHeight))
    {
        RIALTO_SERVER_LOG_MIL("Secondary video playback selected");
        bool westerossinkSecondaryVideoResult = setWesterossinkSecondaryVideo();
        bool ermContextResult = setErmContext();
        if (!westerossinkSecondaryVideoResult && !ermContextResult)
        {
            resetWorkerThread();
            termPipeline();
            throw std::runtime_error("Could not set secondary video");
        }
    }
    else
    {
        RIALTO_SERVER_LOG_MIL("Primary video playback selected");
    }

    m_gstDispatcherThread =
        gstDispatcherThreadFactory->createGstDispatcherThread(*this, m_context.pipeline, m_gstWrapper);
}

GstGenericPlayer::~GstGenericPlayer()
{
    RIALTO_SERVER_LOG_DEBUG("GstGenericPlayer is destructed.");

    m_gstDispatcherThread.reset();

    resetWorkerThread();

    termPipeline();
}

void GstGenericPlayer::initMsePipeline()
{
    // Make playbin
    m_context.pipeline = m_gstWrapper->gstElementFactoryMake("playbin", "media_pipeline");
    // Set pipeline flags
    setAudioVideoFlags(true, true);

    // Set callbacks
    m_glibWrapper->gSignalConnect(m_context.pipeline, "source-setup", G_CALLBACK(&GstGenericPlayer::setupSource), this);
    m_glibWrapper->gSignalConnect(m_context.pipeline, "element-setup", G_CALLBACK(&GstGenericPlayer::setupElement), this);
    m_glibWrapper->gSignalConnect(m_context.pipeline, "deep-element-added",
                                  G_CALLBACK(&GstGenericPlayer::deepElementAdded), this);

    // Set uri
    m_glibWrapper->gObjectSet(m_context.pipeline, "uri", "rialto://", nullptr);

    // Check playsink
    GstElement *playsink = (m_gstWrapper->gstBinGetByName(GST_BIN(m_context.pipeline), "playsink"));
    if (playsink)
    {
        m_glibWrapper->gObjectSet(G_OBJECT(playsink), "send-event-mode", 0, nullptr);
        m_gstWrapper->gstObjectUnref(playsink);
    }
    else
    {
        GST_WARNING("No playsink ?!?!?");
    }
}

void GstGenericPlayer::resetWorkerThread()
{
    // Shutdown task thread
    m_workerThread->enqueueTask(m_taskFactory->createShutdown(*this));
    m_workerThread->join();
    m_workerThread.reset();
}

void GstGenericPlayer::termPipeline()
{
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
    m_gstWrapper->gstObjectUnref(m_context.pipeline);
}

unsigned GstGenericPlayer::getGstPlayFlag(const char *nick)
{
    GFlagsClass *flagsClass =
        static_cast<GFlagsClass *>(m_glibWrapper->gTypeClassRef(m_glibWrapper->gTypeFromName("GstPlayFlags")));
    GFlagsValue *flag = m_glibWrapper->gFlagsGetValueByNick(flagsClass, nick);
    return flag ? flag->value : 0;
}

void GstGenericPlayer::setupSource(GstElement *pipeline, GstElement *source, GstGenericPlayer *self)
{
    self->m_gstWrapper->gstObjectRef(source);
    if (self->m_workerThread)
    {
        self->m_workerThread->enqueueTask(self->m_taskFactory->createSetupSource(self->m_context, *self, source));
    }
}

void GstGenericPlayer::setupElement(GstElement *pipeline, GstElement *element, GstGenericPlayer *self)
{
    RIALTO_SERVER_LOG_DEBUG("Element %s added to the pipeline", GST_ELEMENT_NAME(element));
    self->m_gstWrapper->gstObjectRef(element);
    if (self->m_workerThread)
    {
        self->m_workerThread->enqueueTask(self->m_taskFactory->createSetupElement(self->m_context, *self, element));
    }
}

void GstGenericPlayer::deepElementAdded(GstBin *pipeline, GstBin *bin, GstElement *element, GstGenericPlayer *self)
{
    RIALTO_SERVER_LOG_DEBUG("Deep element %s added to the pipeline", GST_ELEMENT_NAME(element));
    if (self->m_workerThread)
    {
        self->m_workerThread->enqueueTask(
            self->m_taskFactory->createDeepElementAdded(self->m_context, *self, pipeline, bin, element));
    }
}

void GstGenericPlayer::attachSource(const std::unique_ptr<IMediaPipeline::MediaSource> &attachedSource)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createAttachSource(m_context, *this, attachedSource));
    }
}

void GstGenericPlayer::removeSource(const MediaSourceType &mediaSourceType)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createRemoveSource(m_context, *this, mediaSourceType));
    }
}

void GstGenericPlayer::allSourcesAttached()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createFinishSetupSource(m_context, *this));
    }
}

void GstGenericPlayer::attachSamples(const IMediaPipeline::MediaSegmentVector &mediaSegments)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createAttachSamples(m_context, *this, mediaSegments));
    }
}

void GstGenericPlayer::attachSamples(const std::shared_ptr<IDataReader> &dataReader)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createReadShmDataAndAttachSamples(m_context, *this, dataReader));
    }
}

void GstGenericPlayer::setPosition(std::int64_t position)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetPosition(m_context, *this, position));
    }
}

void GstGenericPlayer::setPlaybackRate(double rate)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetPlaybackRate(m_context, rate));
    }
}

bool GstGenericPlayer::getPosition(std::int64_t &position)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstGenericPlayer
    // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
    if (!m_context.pipeline || GST_STATE(m_context.pipeline) < GST_STATE_PAUSED)
    {
        RIALTO_SERVER_LOG_WARN("GetPosition failed. Pipeline is null or state < PAUSED");
        return false;
    }
    if (!m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &position))
    {
        return false;
    }
    return true;
}

bool GstGenericPlayer::getStats(const MediaSourceType &mediaSourceType, uint64_t &renderedFrames, uint64_t &droppedFrames)
{
    bool returnValue{false};
    const char *kSinkName{nullptr};
    switch (mediaSourceType)
    {
    case MediaSourceType::AUDIO:
        kSinkName = "audio-sink";
        break;
    case MediaSourceType::VIDEO:
        kSinkName = "video-sink";
        break;
    default:
        break;
    }
    if (!kSinkName)
    {
        RIALTO_SERVER_LOG_WARN("mediaSourceType not supported %d", static_cast<int>(mediaSourceType));
    }
    else
    {
        GstElement *sink{nullptr};
        m_glibWrapper->gObjectGet(m_context.pipeline, kSinkName, &sink, nullptr);
        if (!sink)
        {
            RIALTO_SERVER_LOG_WARN("%s could not be obtained", kSinkName);
        }
        else
        {
            // For AutoVideoSink we set properties on the child sink
            sink = getSinkChildIfAutoVideoSink(sink);

            GstStructure *stats{nullptr};
            m_glibWrapper->gObjectGet(sink, "stats", &stats, nullptr);
            if (!stats)
            {
                RIALTO_SERVER_LOG_WARN("failed to get stats");
            }
            else
            {
                guint64 renderedFramesTmp;
                guint64 droppedFramesTmp;
                if (m_gstWrapper->gstStructureGetUint64(stats, "rendered", &renderedFramesTmp) &&
                    m_gstWrapper->gstStructureGetUint64(stats, "dropped", &droppedFramesTmp))
                {
                    renderedFrames = renderedFramesTmp;
                    droppedFrames = droppedFramesTmp;
                    returnValue = true;
                }
                else
                {
                    RIALTO_SERVER_LOG_WARN("failed to get stats from structure");
                }
                m_gstWrapper->gstStructureFree(stats);
            }
            m_gstWrapper->gstObjectUnref(GST_OBJECT(sink));
        }
    }

    return returnValue;
}

GstBuffer *GstGenericPlayer::createBuffer(const IMediaPipeline::MediaSegment &mediaSegment) const
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

        uint32_t crypt = 0;
        uint32_t skip = 0;
        bool encryptionPatternSet = mediaSegment.getEncryptionPattern(crypt, skip);

        GstRialtoProtectionData data = {mediaSegment.getMediaKeySessionId(),
                                        static_cast<uint32_t>(mediaSegment.getSubSamples().size()),
                                        mediaSegment.getInitWithLast15(),
                                        keyId,
                                        initVector,
                                        subsamples,
                                        mediaSegment.getCipherMode(),
                                        crypt,
                                        skip,
                                        encryptionPatternSet,
                                        m_context.decryptionService};

        if (!m_protectionMetadataWrapper->addProtectionMetadata(gstBuffer, data))
        {
            RIALTO_SERVER_LOG_ERROR("Failed to add protection metadata");
            if (keyId)
            {
                m_gstWrapper->gstBufferUnref(keyId);
            }
            if (initVector)
            {
                m_gstWrapper->gstBufferUnref(initVector);
            }
            if (subsamples)
            {
                m_gstWrapper->gstBufferUnref(subsamples);
            }
        }
    }

    GST_BUFFER_TIMESTAMP(gstBuffer) = mediaSegment.getTimeStamp();
    GST_BUFFER_DURATION(gstBuffer) = mediaSegment.getDuration();
    return gstBuffer;
}

void GstGenericPlayer::notifyNeedMediaData(bool audioNotificationNeeded, bool videoNotificationNeeded)
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

void GstGenericPlayer::attachAudioData()
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
            m_context.audioAppSrc = elem->second.appSrc;
        }
    }

    if (m_context.audioAppSrc)
    {
        pushSampleIfRequired(m_context.audioAppSrc, "audio");
        if (m_context.audioBuffers.size())
        {
            // This needs to be done before gstAppSrcPushBuffer() is
            // called because it can free the memory
            m_context.lastAudioSampleTimestamps = static_cast<int64_t>(GST_BUFFER_PTS(m_context.audioBuffers.back()));
        }
        for (GstBuffer *buffer : m_context.audioBuffers)
        {
            m_gstWrapper->gstAppSrcPushBuffer(GST_APP_SRC(m_context.audioAppSrc), buffer);
        }
        m_context.audioBuffers.clear();
        m_context.audioDataPushed = true;
        const bool kSingleAudio{m_context.wereAllSourcesAttached &&
                                m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO) ==
                                    m_context.streamInfo.end()};
        if (!m_context.bufferedNotificationSent && (m_context.videoDataPushed || kSingleAudio) && m_gstPlayerClient)
        {
            m_context.bufferedNotificationSent = true;
            m_gstPlayerClient->notifyNetworkState(NetworkState::BUFFERED);
        }
        cancelUnderflow(m_context.audioUnderflowOccured);
    }
}

void GstGenericPlayer::attachVideoData()
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
            m_context.videoAppSrc = elem->second.appSrc;
        }
    }
    if (m_context.videoAppSrc)
    {
        pushSampleIfRequired(m_context.videoAppSrc, "video");
        for (GstBuffer *buffer : m_context.videoBuffers)
        {
            m_gstWrapper->gstAppSrcPushBuffer(GST_APP_SRC(m_context.videoAppSrc), buffer);
        }
        m_context.videoBuffers.clear();
        m_context.videoDataPushed = true;
        const bool kSingleVideo{m_context.wereAllSourcesAttached &&
                                m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO) ==
                                    m_context.streamInfo.end()};
        if (!m_context.bufferedNotificationSent && (m_context.audioDataPushed || kSingleVideo) && m_gstPlayerClient)
        {
            m_context.bufferedNotificationSent = true;
            m_gstPlayerClient->notifyNetworkState(NetworkState::BUFFERED);
        }
        cancelUnderflow(m_context.videoUnderflowOccured);
    }
}

void GstGenericPlayer::updateAudioCaps(int32_t rate, int32_t channels, const std::shared_ptr<CodecData> &codecData)
{
    if (!m_context.audioAppSrc)
    {
        auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO);
        if (elem != m_context.streamInfo.end())
        {
            m_context.audioAppSrc = elem->second.appSrc;
        }
    }

    if (m_context.audioAppSrc)
    {
        constexpr int kInvalidRate{0}, kInvalidChannels{0};
        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(m_context.audioAppSrc));
        GstCaps *newCaps = m_gstWrapper->gstCapsCopy(currentCaps);

        if (rate != kInvalidRate)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "rate", G_TYPE_INT, rate, NULL);
        }

        if (channels != kInvalidChannels)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "channels", G_TYPE_INT, channels, NULL);
        }

        setCodecData(newCaps, codecData);

        if (!m_gstWrapper->gstCapsIsEqual(currentCaps, newCaps))
        {
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(m_context.audioAppSrc), newCaps);
        }

        m_gstWrapper->gstCapsUnref(newCaps);
        m_gstWrapper->gstCapsUnref(currentCaps);
    }
}

void GstGenericPlayer::updateVideoCaps(int32_t width, int32_t height, Fraction frameRate,
                                       const std::shared_ptr<CodecData> &codecData)
{
    if (!m_context.videoAppSrc)
    {
        auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO);
        if (elem != m_context.streamInfo.end())
        {
            m_context.videoAppSrc = elem->second.appSrc;
        }
    }

    if (m_context.videoAppSrc)
    {
        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(m_context.videoAppSrc));
        GstCaps *newCaps = m_gstWrapper->gstCapsCopy(currentCaps);

        if (width > 0)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "width", G_TYPE_INT, width, NULL);
        }

        if (height > 0)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "height", G_TYPE_INT, height, NULL);
        }

        if ((kUndefinedSize != frameRate.numerator) && (kUndefinedSize != frameRate.denominator))
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "framerate", GST_TYPE_FRACTION, frameRate.numerator,
                                           frameRate.denominator, NULL);
        }

        setCodecData(newCaps, codecData);

        if (!m_gstWrapper->gstCapsIsEqual(currentCaps, newCaps))
        {
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(m_context.videoAppSrc), newCaps);
        }

        m_gstWrapper->gstCapsUnref(currentCaps);
        m_gstWrapper->gstCapsUnref(newCaps);
    }
}

void GstGenericPlayer::addAudioClippingToBuffer(GstBuffer *buffer, uint64_t clippingStart, uint64_t clippingEnd) const
{
    if (clippingStart || clippingEnd)
    {
        if (m_gstWrapper->gstBufferAddAudioClippingMeta(buffer, GST_FORMAT_TIME, clippingStart, clippingEnd))
        {
            RIALTO_SERVER_LOG_DEBUG("Added audio clipping to buffer %p, start: %" PRIu64 ", end %" PRIu64, buffer,
                                    clippingStart, clippingEnd);
        }
        else
        {
            RIALTO_SERVER_LOG_WARN("Failed to add audio clipping to buffer %p, start: %" PRIu64 ", end %" PRIu64,
                                   buffer, clippingStart, clippingEnd);
        }
    }
}

bool GstGenericPlayer::setCodecData(GstCaps *caps, const std::shared_ptr<CodecData> &codecData) const
{
    if (codecData && CodecDataType::BUFFER == codecData->type)
    {
        gpointer memory = m_glibWrapper->gMemdup(codecData->data.data(), codecData->data.size());
        GstBuffer *buf = m_gstWrapper->gstBufferNewWrapped(memory, codecData->data.size());
        m_gstWrapper->gstCapsSetSimple(caps, "codec_data", GST_TYPE_BUFFER, buf, nullptr);
        m_gstWrapper->gstBufferUnref(buf);
        return true;
    }
    if (codecData && CodecDataType::STRING == codecData->type)
    {
        std::string codecDataStr(codecData->data.begin(), codecData->data.end());
        m_gstWrapper->gstCapsSetSimple(caps, "codec_data", G_TYPE_STRING, codecDataStr.c_str(), nullptr);
        return true;
    }
    return false;
}

void GstGenericPlayer::pushSampleIfRequired(GstElement *source, const std::string &typeStr)
{
    auto initialPosition = m_context.initialPositions.find(source);
    if (m_context.initialPositions.end() == initialPosition)
    {
        // Sending initial sample not needed
        return;
    }
    RIALTO_SERVER_LOG_DEBUG("Pushing new %s sample...", typeStr.c_str());
    GstSegment *segment{m_gstWrapper->gstSegmentNew()};
    m_gstWrapper->gstSegmentInit(segment, GST_FORMAT_TIME);
    if (!m_gstWrapper->gstSegmentDoSeek(segment, m_context.playbackRate, GST_FORMAT_TIME, GST_SEEK_FLAG_NONE,
                                        GST_SEEK_TYPE_SET, initialPosition->second, GST_SEEK_TYPE_SET,
                                        GST_CLOCK_TIME_NONE, nullptr))
    {
        RIALTO_SERVER_LOG_WARN("Segment seek failed.");
        m_gstWrapper->gstSegmentFree(segment);
        m_context.initialPositions.erase(initialPosition);
        return;
    }

    RIALTO_SERVER_LOG_MIL("New %s segment: [%" GST_TIME_FORMAT ", %" GST_TIME_FORMAT "], rate: %f \n", typeStr.c_str(),
                          GST_TIME_ARGS(segment->start), GST_TIME_ARGS(segment->stop), segment->rate);

    GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(source));
    // We can't pass buffer in GstSample, because implementation of gst_app_src_push_sample
    // uses gst_buffer_copy, which loses RialtoProtectionMeta (that causes problems with EME
    // for first frame).
    GstSample *sample = m_gstWrapper->gstSampleNew(nullptr, currentCaps, segment, nullptr);
    m_gstWrapper->gstAppSrcPushSample(GST_APP_SRC(source), sample);
    m_gstWrapper->gstSampleUnref(sample);
    m_gstWrapper->gstCapsUnref(currentCaps);

    m_gstWrapper->gstSegmentFree(segment);
    m_context.initialPositions.erase(initialPosition);
    return;
}

void GstGenericPlayer::scheduleNeedMediaData(GstAppSrc *src)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createNeedData(m_context, src));
    }
}

void GstGenericPlayer::scheduleEnoughData(GstAppSrc *src)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createEnoughData(m_context, src));
    }
}

void GstGenericPlayer::scheduleAudioUnderflow()
{
    if (m_workerThread)
    {
        bool underflowEnabled = m_context.isPlaying && !m_context.audioSourceRemoved;
        m_workerThread->enqueueTask(m_taskFactory->createUnderflow(m_context, *this, m_context.audioUnderflowOccured,
                                                                   underflowEnabled, MediaSourceType::AUDIO));
    }
}

void GstGenericPlayer::scheduleVideoUnderflow()
{
    if (m_workerThread)
    {
        bool underflowEnabled = m_context.isPlaying;
        m_workerThread->enqueueTask(m_taskFactory->createUnderflow(m_context, *this, m_context.videoUnderflowOccured,
                                                                   underflowEnabled, MediaSourceType::VIDEO));
    }
}

void GstGenericPlayer::scheduleAllSourcesAttached()
{
    allSourcesAttached();
}

void GstGenericPlayer::cancelUnderflow(bool &underflowFlag)
{
    if (!underflowFlag)
    {
        return;
    }
    underflowFlag = false;
}

void GstGenericPlayer::play()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createPlay(*this));
    }
}

void GstGenericPlayer::pause()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createPause(m_context, *this));
    }
}

void GstGenericPlayer::stop()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createStop(m_context, *this));
    }
}

bool GstGenericPlayer::changePipelineState(GstState newState)
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

void GstGenericPlayer::setVideoGeometry(int x, int y, int width, int height)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createSetVideoGeometry(m_context, *this, Rectangle{x, y, width, height}));
    }
}

void GstGenericPlayer::setEos(const firebolt::rialto::MediaSourceType &type)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createEos(m_context, *this, type));
    }
}

bool GstGenericPlayer::setVideoSinkRectangle()
{
    bool result = false;
    GstElement *videoSink = nullptr;
    m_glibWrapper->gObjectGet(m_context.pipeline, "video-sink", &videoSink, nullptr);
    if (videoSink)
    {
        // For AutoVideoSink we set properties on the child sink
        GstElement *actualVideoSink = getSinkChildIfAutoVideoSink(videoSink);
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(actualVideoSink), "rectangle"))
        {
            std::string rect =
                std::to_string(m_context.pendingGeometry.x) + ',' + std::to_string(m_context.pendingGeometry.y) + ',' +
                std::to_string(m_context.pendingGeometry.width) + ',' + std::to_string(m_context.pendingGeometry.height);
            m_glibWrapper->gObjectSet(actualVideoSink, "rectangle", rect.c_str(), nullptr);
            m_context.pendingGeometry.clear();
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set the video rectangle");
        }

        m_gstWrapper->gstObjectUnref(GST_OBJECT(videoSink));
    }

    return result;
}

bool GstGenericPlayer::setWesterossinkSecondaryVideo()
{
    bool result = false;
    GstElementFactory *factory = m_gstWrapper->gstElementFactoryFind("westerossink");
    if (factory)
    {
        GstElement *videoSink = m_gstWrapper->gstElementFactoryCreate(factory, nullptr);
        if (videoSink)
        {
            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), "res-usage"))
            {
                m_glibWrapper->gObjectSet(videoSink, "res-usage", 0x0u, nullptr);
                m_glibWrapper->gObjectSet(m_context.pipeline, "video-sink", videoSink, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set the westerossink res-usage");
                m_gstWrapper->gstObjectUnref(GST_OBJECT(videoSink));
            }
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the westerossink");
        }

        m_gstWrapper->gstObjectUnref(GST_OBJECT(factory));
    }
    else
    {
        // No westeros sink
        result = true;
    }

    return result;
}

bool GstGenericPlayer::setErmContext()
{
    bool result = false;
    GstContext *context = m_gstWrapper->gstContextNew("erm", false);
    if (context)
    {
        GstStructure *contextStructure = m_gstWrapper->gstContextWritableStructure(context);
        if (contextStructure)
        {
            m_gstWrapper->gstStructureSet(contextStructure, "res-usage", G_TYPE_UINT, 0x0u, nullptr);
            m_gstWrapper->gstElementSetContext(GST_ELEMENT(m_context.pipeline), context);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the erm structure");
        }
        m_gstWrapper->gstContextUnref(context);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the erm context");
    }

    return result;
}

void GstGenericPlayer::startPositionReportingAndCheckAudioUnderflowTimer()
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

void GstGenericPlayer::stopPositionReportingAndCheckAudioUnderflowTimer()
{
    if (m_positionReportingAndCheckAudioUnderflowTimer && m_positionReportingAndCheckAudioUnderflowTimer->isActive())
    {
        m_positionReportingAndCheckAudioUnderflowTimer->cancel();
        m_positionReportingAndCheckAudioUnderflowTimer.reset();
    }
}

void GstGenericPlayer::stopWorkerThread()
{
    if (m_workerThread)
    {
        m_workerThread->stop();
    }
}

void GstGenericPlayer::setPendingPlaybackRate()
{
    RIALTO_SERVER_LOG_INFO("Setting pending playback rate");
    setPlaybackRate(m_context.pendingPlaybackRate);
}

void GstGenericPlayer::renderFrame()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createRenderFrame(m_context, *this));
    }
}

void GstGenericPlayer::setVolume(double volume)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetVolume(m_context, volume));
    }
}

bool GstGenericPlayer::getVolume(double &volume)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstGenericPlayer
    // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
    if (!m_context.pipeline)
    {
        return false;
    }
    volume =
        m_gstWrapper->gstStreamVolumeGetVolume(GST_STREAM_VOLUME(m_context.pipeline), GST_STREAM_VOLUME_FORMAT_LINEAR);
    return true;
}

void GstGenericPlayer::setMute(bool mute)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetMute(m_context, mute));
    }
}

bool GstGenericPlayer::getMute(bool &mute)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstGenericPlayer
    // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
    if (!m_context.pipeline)
    {
        return false;
    }
    mute = m_gstWrapper->gstStreamVolumeGetMute(GST_STREAM_VOLUME(m_context.pipeline));
    return true;
}

void GstGenericPlayer::ping(std::unique_ptr<IHeartbeatHandler> &&heartbeatHandler)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createPing(std::move(heartbeatHandler)));
    }
}

void GstGenericPlayer::flush(const MediaSourceType &mediaSourceType, bool resetTime)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createFlush(m_context, mediaSourceType, resetTime));
    }
}

void GstGenericPlayer::setSourcePosition(const MediaSourceType &mediaSourceType, int64_t position)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSourcePosition(m_context, mediaSourceType, position));
    }
}

void GstGenericPlayer::processAudioGap(int64_t position, uint32_t duration, int64_t discontinuityGap, bool audioAac)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createProcessAudioGap(m_context, position, duration, discontinuityGap, audioAac));
    }
}

void GstGenericPlayer::handleBusMessage(GstMessage *message)
{
    m_workerThread->enqueueTask(m_taskFactory->createHandleBusMessage(m_context, *this, message));
}

void GstGenericPlayer::updatePlaybackGroup(GstElement *typefind, const GstCaps *caps)
{
    m_workerThread->enqueueTask(m_taskFactory->createUpdatePlaybackGroup(m_context, typefind, caps));
}

void GstGenericPlayer::addAutoVideoSinkChild(GObject *object)
{
    // Only add children that are sinks
    if (GST_OBJECT_FLAG_IS_SET(GST_ELEMENT(object), GST_ELEMENT_FLAG_SINK))
    {
        RIALTO_SERVER_LOG_DEBUG("Store AutoVideoSink child sink");

        if (m_context.autoVideoChildSink && m_context.autoVideoChildSink != GST_ELEMENT(object))
        {
            RIALTO_SERVER_LOG_MIL("AutoVideoSink child is been overwritten");
        }
        m_context.autoVideoChildSink = GST_ELEMENT(object);
    }
}

void GstGenericPlayer::removeAutoVideoSinkChild(GObject *object)
{
    if (GST_OBJECT_FLAG_IS_SET(GST_ELEMENT(object), GST_ELEMENT_FLAG_SINK))
    {
        RIALTO_SERVER_LOG_DEBUG("Remove AutoVideoSink child sink");

        if (m_context.autoVideoChildSink && m_context.autoVideoChildSink != GST_ELEMENT(object))
        {
            RIALTO_SERVER_LOG_MIL("AutoVideoSink child sink is not the same as the one stored");
            return;
        }

        m_context.autoVideoChildSink = nullptr;
    }
}

GstElement *GstGenericPlayer::getSinkChildIfAutoVideoSink(GstElement *sink)
{
    const gchar *kTmpName = m_glibWrapper->gTypeName(G_OBJECT_TYPE(sink));
    if (!kTmpName)
        return sink;

    const std::string kElementTypeName{kTmpName};
    if (kElementTypeName == "GstAutoVideoSink")
    {
        if (!m_context.autoVideoChildSink)
        {
            RIALTO_SERVER_LOG_WARN("No child sink has been added to the autovideosink");
            return sink;
        }
        else
        {
            return m_context.autoVideoChildSink;
        }
    }
    else
    {
        return sink;
    }
}

void GstGenericPlayer::setAudioVideoFlags(bool enableAudio, bool enableVideo)
{
    unsigned flagAudio{0};
    unsigned flagNativeAudio{0};
    unsigned flagVideo{0};
    unsigned flagNativeVideo{0};
    if (enableAudio)
    {
        flagAudio = getGstPlayFlag("audio");
        flagNativeAudio = shouldEnableNativeAudio() ? getGstPlayFlag("native-audio") : 0;
    }
    if (enableVideo)
    {
        flagVideo = getGstPlayFlag("video");
        flagNativeVideo = getGstPlayFlag("native-video");
    }
    m_glibWrapper->gObjectSet(m_context.pipeline, "flags", flagAudio | flagVideo | flagNativeVideo | flagNativeAudio,
                              nullptr);
}

bool GstGenericPlayer::shouldEnableNativeAudio()
{
    GstElementFactory *factory = m_gstWrapper->gstElementFactoryFind("brcmaudiosink");
    if (factory)
    {
        m_gstWrapper->gstObjectUnref(GST_OBJECT(factory));
        return true;
    }
    return false;
}

}; // namespace firebolt::rialto::server
