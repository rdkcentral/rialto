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
#include "IGstTextTrackSinkFactory.h"
#include "IMediaPipeline.h"
#include "ITimer.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"
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
        gstPlayer = std::make_unique<
            GstGenericPlayer>(client, decryptionService, type, videoRequirements, gstWrapper, glibWrapper,
                              IGstSrcFactory::getFactory(), common::ITimerFactory::getFactory(),
                              std::make_unique<GenericPlayerTaskFactory>(client, gstWrapper, glibWrapper,
                                                                         rdkGstreamerUtilsWrapper,
                                                                         IGstTextTrackSinkFactory::createFactory()),
                              std::make_unique<WorkerThreadFactory>(), std::make_unique<GstDispatcherThreadFactory>(),
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
    setPlaybinFlags(true);

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

    for (auto &elem : m_context.streamInfo)
    {
        StreamInfo &streamInfo = elem.second;
        for (auto &buffer : streamInfo.buffers)
        {
            m_gstWrapper->gstBufferUnref(buffer);
        }

        streamInfo.buffers.clear();
    }

    m_taskFactory->createStop(m_context, *this)->execute();
    GstBus *bus = m_gstWrapper->gstPipelineGetBus(GST_PIPELINE(m_context.pipeline));
    m_gstWrapper->gstBusSetSyncHandler(bus, nullptr, nullptr, nullptr);
    m_gstWrapper->gstObjectUnref(bus);

    if (m_context.source)
    {
        m_gstWrapper->gstObjectUnref(m_context.source);
    }
    if (m_context.subtitleSink)
    {
        m_gstWrapper->gstObjectUnref(m_context.subtitleSink);
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

GstElement *GstGenericPlayer::getSink(const MediaSourceType &mediaSourceType) const
{
    const char *kSinkName{nullptr};
    GstElement *sink{nullptr};
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
        if (m_context.pipeline == nullptr)
        {
            RIALTO_SERVER_LOG_WARN("Pipeline is NULL!");
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pipeline is valid: %p", m_context.pipeline);
        }
        m_glibWrapper->gObjectGet(m_context.pipeline, kSinkName, &sink, nullptr);
        if (sink)
        {
            GstElement *autoSink{sink};
            if (firebolt::rialto::MediaSourceType::VIDEO == mediaSourceType)
                autoSink = getSinkChildIfAutoVideoSink(sink);
            else if (firebolt::rialto::MediaSourceType::AUDIO == mediaSourceType)
                autoSink = getSinkChildIfAutoAudioSink(sink);

            // Is this an auto-sink?...
            if (autoSink != sink)
            {
                m_gstWrapper->gstObjectUnref(GST_OBJECT(sink));

                // increase the reference count of the auto sink
                sink = GST_ELEMENT(m_gstWrapper->gstObjectRef(GST_OBJECT(autoSink)));
            }
        }
        else
        {
            RIALTO_SERVER_LOG_WARN("%s could not be obtained", kSinkName);
        }
    }
    return sink;
}

GstElement *GstGenericPlayer::getDecoder(const MediaSourceType &mediaSourceType)
{
    GstIterator *it = m_gstWrapper->gstBinIterateRecurse(GST_BIN(m_context.pipeline));
    GValue item = G_VALUE_INIT;
    gboolean done = FALSE;

    while (!done)
    {
        switch (m_gstWrapper->gstIteratorNext(it, &item))
        {
        case GST_ITERATOR_OK:
        {
            GstElement *element = GST_ELEMENT(m_glibWrapper->gValueGetObject(&item));
            GstElementFactory *factory = m_gstWrapper->gstElementGetFactory(element);

            if (factory)
            {
                GstElementFactoryListType type = GST_ELEMENT_FACTORY_TYPE_DECODER;
                if (mediaSourceType == MediaSourceType::AUDIO)
                {
                    type |= GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO;
                }
                else if (mediaSourceType == MediaSourceType::VIDEO)
                {
                    type |= GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO;
                }

                if (m_gstWrapper->gstElementFactoryListIsType(factory, type))
                {
                    m_glibWrapper->gValueUnset(&item);
                    m_gstWrapper->gstIteratorFree(it);
                    return GST_ELEMENT(m_gstWrapper->gstObjectRef(element));
                }
            }

            m_glibWrapper->gValueUnset(&item);
            break;
        }
        case GST_ITERATOR_RESYNC:
            m_gstWrapper->gstIteratorResync(it);
            break;
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        }
    }

    RIALTO_SERVER_LOG_WARN("Could not find decoder");

    m_glibWrapper->gValueUnset(&item);
    m_gstWrapper->gstIteratorFree(it);

    return nullptr;
}

GstElement *GstGenericPlayer::getParser(const MediaSourceType &mediaSourceType)
{
    GstIterator *it = m_gstWrapper->gstBinIterateRecurse(GST_BIN(m_context.pipeline));
    GValue item = G_VALUE_INIT;
    gboolean done = FALSE;

    while (!done)
    {
        switch (m_gstWrapper->gstIteratorNext(it, &item))
        {
        case GST_ITERATOR_OK:
        {
            GstElement *element = GST_ELEMENT(m_glibWrapper->gValueGetObject(&item));
            GstElementFactory *factory = m_gstWrapper->gstElementGetFactory(element);

            if (factory)
            {
                GstElementFactoryListType type = GST_ELEMENT_FACTORY_TYPE_PARSER;
                if (mediaSourceType == MediaSourceType::AUDIO)
                {
                    type |= GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO;
                }
                else if (mediaSourceType == MediaSourceType::VIDEO)
                {
                    type |= GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO;
                }

                if (m_gstWrapper->gstElementFactoryListIsType(factory, type))
                {
                    m_glibWrapper->gValueUnset(&item);
                    m_gstWrapper->gstIteratorFree(it);
                    return GST_ELEMENT(m_gstWrapper->gstObjectRef(element));
                }
            }

            m_glibWrapper->gValueUnset(&item);
            break;
        }
        case GST_ITERATOR_RESYNC:
            m_gstWrapper->gstIteratorResync(it);
            break;
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        }
    }

    RIALTO_SERVER_LOG_WARN("Could not find parser");

    m_glibWrapper->gValueUnset(&item);
    m_gstWrapper->gstIteratorFree(it);

    return nullptr;
}

bool GstGenericPlayer::setImmediateOutput(const MediaSourceType &mediaSourceType, bool immediateOutputParam)
{
    if (!m_workerThread)
        return false;

    m_workerThread->enqueueTask(
        m_taskFactory->createSetImmediateOutput(m_context, *this, mediaSourceType, immediateOutputParam));
    return true;
}

bool GstGenericPlayer::getImmediateOutput(const MediaSourceType &mediaSourceType, bool &immediateOutputRef)
{
    bool returnValue{false};
    GstElement *sink{getSink(mediaSourceType)};
    if (sink)
    {
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "immediate-output"))
        {
            m_glibWrapper->gObjectGet(sink, "immediate-output", &immediateOutputRef, nullptr);
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("immediate-output not supported in element %s", GST_ELEMENT_NAME(sink));
        }
        m_gstWrapper->gstObjectUnref(sink);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set immediate-output property, sink is NULL");
    }

    return returnValue;
}

bool GstGenericPlayer::getStats(const MediaSourceType &mediaSourceType, uint64_t &renderedFrames, uint64_t &droppedFrames)
{
    bool returnValue{false};
    GstElement *sink{getSink(mediaSourceType)};
    if (sink)
    {
        GstStructure *stats{nullptr};
        m_glibWrapper->gObjectGet(sink, "stats", &stats, nullptr);
        if (!stats)
        {
            RIALTO_SERVER_LOG_ERROR("failed to get stats from '%s'", GST_ELEMENT_NAME(sink));
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
                RIALTO_SERVER_LOG_ERROR("failed to get 'rendered' or 'dropped' from structure (%s)",
                                        GST_ELEMENT_NAME(sink));
            }
            m_gstWrapper->gstStructureFree(stats);
        }
        m_gstWrapper->gstObjectUnref(sink);
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
        GstBuffer *subsamples{nullptr};
        if (!mediaSegment.getSubSamples().empty())
        {
            auto subsamplesRawSize = mediaSegment.getSubSamples().size() * (sizeof(guint16) + sizeof(guint32));
            guint8 *subsamplesRaw = static_cast<guint8 *>(m_glibWrapper->gMalloc(subsamplesRawSize));
            GstByteWriter writer;
            m_gstWrapper->gstByteWriterInitWithData(&writer, subsamplesRaw, subsamplesRawSize, FALSE);

            for (const auto &subSample : mediaSegment.getSubSamples())
            {
                m_gstWrapper->gstByteWriterPutUint16Be(&writer, subSample.numClearBytes);
                m_gstWrapper->gstByteWriterPutUint32Be(&writer, subSample.numEncryptedBytes);
            }
            subsamples = m_gstWrapper->gstBufferNewWrapped(subsamplesRaw, subsamplesRawSize);
        }

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

void GstGenericPlayer::notifyNeedMediaData(const MediaSourceType mediaSource)
{
    auto elem = m_context.streamInfo.find(mediaSource);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;
        streamInfo.isNeedDataPending = false;

        // Send new NeedMediaData if we still need it
        if (m_gstPlayerClient && streamInfo.isDataNeeded)
        {
            streamInfo.isNeedDataPending = m_gstPlayerClient->notifyNeedMediaData(mediaSource);
        }
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("Media type %s could not be found", common::convertMediaSourceType(mediaSource));
    }
}

void GstGenericPlayer::attachData(const firebolt::rialto::MediaSourceType mediaType)
{
    auto elem = m_context.streamInfo.find(mediaType);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;
        if (streamInfo.buffers.empty() || !streamInfo.isDataNeeded)
        {
            return;
        }

        pushSampleIfRequired(streamInfo.appSrc, common::convertMediaSourceType(mediaType));
        if (mediaType == firebolt::rialto::MediaSourceType::AUDIO)
        {
            // This needs to be done before gstAppSrcPushBuffer() is
            // called because it can free the memory
            m_context.lastAudioSampleTimestamps = static_cast<int64_t>(GST_BUFFER_PTS(streamInfo.buffers.back()));
        }

        for (GstBuffer *buffer : streamInfo.buffers)
        {
            m_gstWrapper->gstAppSrcPushBuffer(GST_APP_SRC(streamInfo.appSrc), buffer);
        }
        streamInfo.buffers.clear();
        streamInfo.isDataPushed = true;

        const bool kIsSingle = m_context.streamInfo.size() == 1;
        bool allOtherStreamsPushed = std::all_of(m_context.streamInfo.begin(), m_context.streamInfo.end(),
                                                 [](const auto &entry) { return entry.second.isDataPushed; });

        if (!m_context.bufferedNotificationSent && (allOtherStreamsPushed || kIsSingle) && m_gstPlayerClient)
        {
            m_context.bufferedNotificationSent = true;
            m_gstPlayerClient->notifyNetworkState(NetworkState::BUFFERED);
        }
        cancelUnderflow(mediaType);
    }
}

void GstGenericPlayer::updateAudioCaps(int32_t rate, int32_t channels, const std::shared_ptr<CodecData> &codecData)
{
    auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;

        constexpr int kInvalidRate{0}, kInvalidChannels{0};
        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(streamInfo.appSrc));
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
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(streamInfo.appSrc), newCaps);
        }

        m_gstWrapper->gstCapsUnref(newCaps);
        m_gstWrapper->gstCapsUnref(currentCaps);
    }
}

void GstGenericPlayer::updateVideoCaps(int32_t width, int32_t height, Fraction frameRate,
                                       const std::shared_ptr<CodecData> &codecData)
{
    auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;

        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(streamInfo.appSrc));
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
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(streamInfo.appSrc), newCaps);
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
    for (const auto &[position, resetTime, appliedRate, stopPosition] : initialPosition->second)
    {
        GstSeekFlags seekFlag = resetTime ? GST_SEEK_FLAG_FLUSH : GST_SEEK_FLAG_NONE;
        RIALTO_SERVER_LOG_DEBUG("Pushing new %s sample...", typeStr.c_str());
        GstSegment *segment{m_gstWrapper->gstSegmentNew()};
        m_gstWrapper->gstSegmentInit(segment, GST_FORMAT_TIME);
        if (!m_gstWrapper->gstSegmentDoSeek(segment, m_context.playbackRate, GST_FORMAT_TIME, seekFlag,
                                            GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_SET, stopPosition, nullptr))
        {
            RIALTO_SERVER_LOG_WARN("Segment seek failed.");
            m_gstWrapper->gstSegmentFree(segment);
            m_context.initialPositions.erase(initialPosition);
            return;
        }
        segment->applied_rate = appliedRate;
        RIALTO_SERVER_LOG_MIL("New %s segment: [%" GST_TIME_FORMAT ", %" GST_TIME_FORMAT
                              "], rate: %f, appliedRate %f, reset_time: %d\n",
                              typeStr.c_str(), GST_TIME_ARGS(segment->start), GST_TIME_ARGS(segment->stop),
                              segment->rate, segment->applied_rate, resetTime);

        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(source));
        // We can't pass buffer in GstSample, because implementation of gst_app_src_push_sample
        // uses gst_buffer_copy, which loses RialtoProtectionMeta (that causes problems with EME
        // for first frame).
        GstSample *sample = m_gstWrapper->gstSampleNew(nullptr, currentCaps, segment, nullptr);
        m_gstWrapper->gstAppSrcPushSample(GST_APP_SRC(source), sample);
        m_gstWrapper->gstSampleUnref(sample);
        m_gstWrapper->gstCapsUnref(currentCaps);

        m_gstWrapper->gstSegmentFree(segment);
    }
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
        m_workerThread->enqueueTask(
            m_taskFactory->createUnderflow(m_context, *this, underflowEnabled, MediaSourceType::AUDIO));
    }
}

void GstGenericPlayer::scheduleVideoUnderflow()
{
    if (m_workerThread)
    {
        bool underflowEnabled = m_context.isPlaying;
        m_workerThread->enqueueTask(
            m_taskFactory->createUnderflow(m_context, *this, underflowEnabled, MediaSourceType::VIDEO));
    }
}

void GstGenericPlayer::scheduleAllSourcesAttached()
{
    allSourcesAttached();
}

void GstGenericPlayer::cancelUnderflow(firebolt::rialto::MediaSourceType mediaSource)
{
    auto elem = m_context.streamInfo.find(mediaSource);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;
        if (!streamInfo.underflowOccured)
        {
            return;
        }

        RIALTO_SERVER_LOG_DEBUG("Cancelling %s underflow", common::convertMediaSourceType(mediaSource));
        streamInfo.underflowOccured = false;
    }
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
    GstElement *videoSink{getSink(MediaSourceType::VIDEO)};
    if (videoSink)
    {
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), "rectangle"))
        {
            std::string rect =
                std::to_string(m_context.pendingGeometry.x) + ',' + std::to_string(m_context.pendingGeometry.y) + ',' +
                std::to_string(m_context.pendingGeometry.width) + ',' + std::to_string(m_context.pendingGeometry.height);
            m_glibWrapper->gObjectSet(videoSink, "rectangle", rect.c_str(), nullptr);
            m_context.pendingGeometry.clear();
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set the video rectangle");
        }
        m_gstWrapper->gstObjectUnref(videoSink);
    }

    return result;
}

bool GstGenericPlayer::setImmediateOutput()
{
    bool result{false};
    if (m_context.pendingImmediateOutputForVideo.has_value())
    {
        GstElement *sink{getSink(MediaSourceType::VIDEO)};
        if (sink)
        {
            bool immediateOutput{m_context.pendingImmediateOutputForVideo.value()};
            RIALTO_SERVER_LOG_DEBUG("Set immediate-output to %s", immediateOutput ? "TRUE" : "FALSE");

            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "immediate-output"))
            {
                gboolean immediateOutputGboolean{immediateOutput ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(sink, "immediate-output", immediateOutputGboolean, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set immediate-output property on sink '%s'", GST_ELEMENT_NAME(sink));
            }
            m_context.pendingImmediateOutputForVideo.reset();
            m_gstWrapper->gstObjectUnref(sink);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending an immediate-output, sink is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setLowLatency()
{
    bool result{false};
    if (m_context.pendingLowLatency.has_value())
    {
        GstElement *sink{getSink(MediaSourceType::AUDIO)};
        if (sink)
        {
            bool lowLatency{m_context.pendingLowLatency.value()};
            RIALTO_SERVER_LOG_DEBUG("Set low-latency to %s", lowLatency ? "TRUE" : "FALSE");

            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "low-latency"))
            {
                gboolean lowLatencyGboolean{lowLatency ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(sink, "low-latency", lowLatencyGboolean, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set low-latency property on sink '%s'", GST_ELEMENT_NAME(sink));
            }
            m_context.pendingLowLatency.reset();
            m_gstWrapper->gstObjectUnref(sink);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending low-latency, sink is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setSync()
{
    bool result{false};
    if (m_context.pendingSync.has_value())
    {
        GstElement *sink{getSink(MediaSourceType::AUDIO)};
        if (sink)
        {
            bool sync{m_context.pendingSync.value()};
            RIALTO_SERVER_LOG_DEBUG("Set sync to %s", sync ? "TRUE" : "FALSE");

            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "sync"))
            {
                gboolean syncGboolean{sync ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(sink, "sync", syncGboolean, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set sync property on sink '%s'", GST_ELEMENT_NAME(sink));
            }
            m_context.pendingSync.reset();
            m_gstWrapper->gstObjectUnref(sink);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending sync, sink is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setSyncOff()
{
    bool result{false};
    if (m_context.pendingSyncOff.has_value())
    {
        GstElement *decoder = getDecoder(MediaSourceType::AUDIO);
        if (decoder)
        {
            bool syncOff{m_context.pendingSyncOff.value()};
            RIALTO_SERVER_LOG_DEBUG("Set sync-off to %s", syncOff ? "TRUE" : "FALSE");

            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "sync-off"))
            {
                gboolean syncOffGboolean{decoder ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(decoder, "sync-off", syncOffGboolean, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set sync-off property on decoder '%s'", GST_ELEMENT_NAME(decoder));
            }
            m_context.pendingSyncOff.reset();
            m_gstWrapper->gstObjectUnref(decoder);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending sync-off, decoder is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setStreamSyncMode(const MediaSourceType &type)
{
    bool result{false};
    int32_t streamSyncMode{0};
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (m_context.pendingStreamSyncMode.find(type) == m_context.pendingStreamSyncMode.end())
        {
            return false;
        }
        streamSyncMode = m_context.pendingStreamSyncMode[type];
    }
    if (MediaSourceType::AUDIO == type)
    {
        GstElement *decoder = getDecoder(MediaSourceType::AUDIO);
        if (!decoder)
        {
            RIALTO_SERVER_LOG_DEBUG("Pending stream-sync-mode, decoder is NULL");
            return false;
        }

        RIALTO_SERVER_LOG_DEBUG("Set stream-sync-mode to %d", streamSyncMode);

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "stream-sync-mode"))
        {
            gint streamSyncModeGint{static_cast<gint>(streamSyncMode)};
            m_glibWrapper->gObjectSet(decoder, "stream-sync-mode", streamSyncModeGint, nullptr);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set stream-sync-mode property on decoder '%s'", GST_ELEMENT_NAME(decoder));
        }
        m_gstWrapper->gstObjectUnref(decoder);
        std::unique_lock lock{m_context.propertyMutex};
        m_context.pendingStreamSyncMode.erase(type);
    }
    else if (MediaSourceType::VIDEO == type)
    {
        GstElement *parser = getParser(MediaSourceType::VIDEO);
        if (!parser)
        {
            RIALTO_SERVER_LOG_DEBUG("Pending syncmode-streaming, parser is NULL");
            return false;
        }

        gboolean streamSyncModeBoolean{static_cast<gboolean>(streamSyncMode)};
        RIALTO_SERVER_LOG_DEBUG("Set syncmode-streaming to %d", streamSyncMode);

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(parser), "syncmode-streaming"))
        {
            m_glibWrapper->gObjectSet(parser, "syncmode-streaming", streamSyncModeBoolean, nullptr);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set syncmode-streaming property on parser '%s'", GST_ELEMENT_NAME(parser));
        }
        m_gstWrapper->gstObjectUnref(parser);
        std::unique_lock lock{m_context.propertyMutex};
        m_context.pendingStreamSyncMode.erase(type);
    }
    return result;
}

bool GstGenericPlayer::setRenderFrame()
{
    bool result{false};
    if (m_context.pendingRenderFrame)
    {
        static const std::string kStepOnPrerollPropertyName = "frame-step-on-preroll";
        GstElement *sink{getSink(MediaSourceType::VIDEO)};
        if (sink)
        {
            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), kStepOnPrerollPropertyName.c_str()))
            {
                RIALTO_SERVER_LOG_INFO("Rendering preroll");

                m_glibWrapper->gObjectSet(sink, kStepOnPrerollPropertyName.c_str(), 1, nullptr);
                m_gstWrapper->gstElementSendEvent(sink, m_gstWrapper->gstEventNewStep(GST_FORMAT_BUFFERS, 1, 1.0, true,
                                                                                      false));
                m_glibWrapper->gObjectSet(sink, kStepOnPrerollPropertyName.c_str(), 0, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Video sink doesn't have property `%s`", kStepOnPrerollPropertyName.c_str());
            }
            m_gstWrapper->gstObjectUnref(sink);
            m_context.pendingRenderFrame = false;
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending render frame, sink is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setBufferingLimit()
{
    bool result{false};
    guint bufferingLimit{0};
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (!m_context.pendingBufferingLimit.has_value())
        {
            return false;
        }
        bufferingLimit = static_cast<guint>(m_context.pendingBufferingLimit.value());
    }

    GstElement *decoder{getDecoder(MediaSourceType::AUDIO)};
    if (decoder)
    {
        RIALTO_SERVER_LOG_DEBUG("Set limit-buffering-ms to %u", bufferingLimit);

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "limit-buffering-ms"))
        {
            m_glibWrapper->gObjectSet(decoder, "limit-buffering-ms", bufferingLimit, nullptr);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set limit-buffering-ms property on decoder '%s'",
                                    GST_ELEMENT_NAME(decoder));
        }
        m_gstWrapper->gstObjectUnref(decoder);
        std::unique_lock lock{m_context.propertyMutex};
        m_context.pendingBufferingLimit.reset();
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Pending limit-buffering-ms, decoder is NULL");
    }
    return result;
}

bool GstGenericPlayer::setUseBuffering()
{
    std::unique_lock lock{m_context.propertyMutex};
    if (m_context.pendingUseBuffering.has_value())
    {
        if (m_context.playbackGroup.m_curAudioDecodeBin)
        {
            gboolean useBufferingGboolean{m_context.pendingUseBuffering.value() ? TRUE : FALSE};
            RIALTO_SERVER_LOG_DEBUG("Set use-buffering to %d", useBufferingGboolean);
            m_glibWrapper->gObjectSet(m_context.playbackGroup.m_curAudioDecodeBin, "use-buffering",
                                      useBufferingGboolean, nullptr);
            m_context.pendingUseBuffering.reset();
            return true;
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending use-buffering, decodebin is NULL");
        }
    }
    return false;
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

void GstGenericPlayer::setVolume(double targetVolume, uint32_t volumeDuration, firebolt::rialto::EaseType easeType)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createSetVolume(m_context, *this, targetVolume, volumeDuration, easeType));
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

void GstGenericPlayer::setMute(const MediaSourceType &mediaSourceType, bool mute)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetMute(m_context, mediaSourceType, mute));
    }
}

bool GstGenericPlayer::getMute(const MediaSourceType &mediaSourceType, bool &mute)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstGenericPlayer
    // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
    if (mediaSourceType == MediaSourceType::SUBTITLE)
    {
        if (!m_context.subtitleSink)
        {
            RIALTO_SERVER_LOG_ERROR("There is no subtitle sink");
            return false;
        }
        gboolean muteValue{FALSE};
        m_glibWrapper->gObjectGet(m_context.subtitleSink, "mute", &muteValue, nullptr);
        mute = muteValue;
    }
    else if (mediaSourceType == MediaSourceType::AUDIO)
    {
        if (!m_context.pipeline)
        {
            return false;
        }
        mute = m_gstWrapper->gstStreamVolumeGetMute(GST_STREAM_VOLUME(m_context.pipeline));
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Getting mute for type %s unsupported", common::convertMediaSourceType(mediaSourceType));
        return false;
    }

    return true;
}

void GstGenericPlayer::setTextTrackIdentifier(const std::string &textTrackIdentifier)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetTextTrackIdentifier(m_context, textTrackIdentifier));
    }
}

bool GstGenericPlayer::getTextTrackIdentifier(std::string &textTrackIdentifier)
{
    if (!m_context.subtitleSink)
    {
        RIALTO_SERVER_LOG_ERROR("There is no subtitle sink");
        return false;
    }

    gchar *identifier = nullptr;
    m_glibWrapper->gObjectGet(m_context.subtitleSink, "text-track-identifier", &identifier, nullptr);

    if (identifier)
    {
        textTrackIdentifier = identifier;
        m_glibWrapper->gFree(identifier);
        return true;
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get text track identifier");
        return false;
    }
}

bool GstGenericPlayer::setLowLatency(bool lowLatency)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetLowLatency(m_context, *this, lowLatency));
    }
    return true;
}

bool GstGenericPlayer::setSync(bool sync)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSync(m_context, *this, sync));
    }
    return true;
}

bool GstGenericPlayer::getSync(bool &sync)
{
    bool returnValue{false};
    GstElement *sink{getSink(MediaSourceType::AUDIO)};
    if (sink)
    {
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "sync"))
        {
            m_glibWrapper->gObjectGet(sink, "sync", &sync, nullptr);
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Sync not supported in sink '%s'", GST_ELEMENT_NAME(sink));
        }
        m_gstWrapper->gstObjectUnref(sink);
    }
    else if (m_context.pendingSync.has_value())
    {
        RIALTO_SERVER_LOG_DEBUG("Returning queued value");
        sync = m_context.pendingSync.value();
        returnValue = true;
    }
    else
    {
        // We dont know the default setting on the sync, so return failure here
        RIALTO_SERVER_LOG_WARN("No audio sink attached or queued value");
    }

    return returnValue;
}

bool GstGenericPlayer::setSyncOff(bool syncOff)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSyncOff(m_context, *this, syncOff));
    }
    return true;
}

bool GstGenericPlayer::setStreamSyncMode(const MediaSourceType &mediaSourceType, int32_t streamSyncMode)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createSetStreamSyncMode(m_context, *this, mediaSourceType, streamSyncMode));
    }
    return true;
}

bool GstGenericPlayer::getStreamSyncMode(int32_t &streamSyncMode)
{
    bool returnValue{false};
    GstElement *decoder = getDecoder(MediaSourceType::AUDIO);
    if (decoder && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "stream-sync-mode"))
    {
        m_glibWrapper->gObjectGet(decoder, "stream-sync-mode", &streamSyncMode, nullptr);
        returnValue = true;
    }
    else
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (m_context.pendingStreamSyncMode.find(MediaSourceType::AUDIO) != m_context.pendingStreamSyncMode.end())
        {
            RIALTO_SERVER_LOG_DEBUG("Returning queued value");
            streamSyncMode = m_context.pendingStreamSyncMode[MediaSourceType::AUDIO];
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Stream sync mode not supported in decoder '%s'",
                                    (decoder ? GST_ELEMENT_NAME(decoder) : "null"));
        }
    }

    if (decoder)
        m_gstWrapper->gstObjectUnref(GST_OBJECT(decoder));

    return returnValue;
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

void GstGenericPlayer::setSourcePosition(const MediaSourceType &mediaSourceType, int64_t position, bool resetTime,
                                         double appliedRate, uint64_t stopPosition)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSourcePosition(m_context, *this, mediaSourceType, position,
                                                                           resetTime, appliedRate, stopPosition));
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

void GstGenericPlayer::setBufferingLimit(uint32_t limitBufferingMs)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetBufferingLimit(m_context, *this, limitBufferingMs));
    }
}

bool GstGenericPlayer::getBufferingLimit(uint32_t &limitBufferingMs)
{
    bool returnValue{false};
    GstElement *decoder = getDecoder(MediaSourceType::AUDIO);
    if (decoder && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "limit-buffering-ms"))
    {
        m_glibWrapper->gObjectGet(decoder, "limit-buffering-ms", &limitBufferingMs, nullptr);
        returnValue = true;
    }
    else
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (m_context.pendingBufferingLimit.has_value())
        {
            RIALTO_SERVER_LOG_DEBUG("Returning queued value");
            limitBufferingMs = m_context.pendingBufferingLimit.value();
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("buffering limit not supported in decoder '%s'",
                                    (decoder ? GST_ELEMENT_NAME(decoder) : "null"));
        }
    }

    if (decoder)
        m_gstWrapper->gstObjectUnref(GST_OBJECT(decoder));

    return returnValue;
}

void GstGenericPlayer::setUseBuffering(bool useBuffering)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetUseBuffering(m_context, *this, useBuffering));
    }
}

bool GstGenericPlayer::getUseBuffering(bool &useBuffering)
{
    if (m_context.playbackGroup.m_curAudioDecodeBin)
    {
        m_glibWrapper->gObjectGet(m_context.playbackGroup.m_curAudioDecodeBin, "use-buffering", &useBuffering, nullptr);
        return true;
    }
    else
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (m_context.pendingUseBuffering.has_value())
        {
            RIALTO_SERVER_LOG_DEBUG("Returning queued value");
            useBuffering = m_context.pendingUseBuffering.value();
            return true;
        }
    }
    return false;
}

void GstGenericPlayer::handleBusMessage(GstMessage *message)
{
    m_workerThread->enqueueTask(m_taskFactory->createHandleBusMessage(m_context, *this, message));
}

void GstGenericPlayer::updatePlaybackGroup(GstElement *typefind, const GstCaps *caps)
{
    m_workerThread->enqueueTask(m_taskFactory->createUpdatePlaybackGroup(m_context, *this, typefind, caps));
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

void GstGenericPlayer::addAutoAudioSinkChild(GObject *object)
{
    // Only add children that are sinks
    if (GST_OBJECT_FLAG_IS_SET(GST_ELEMENT(object), GST_ELEMENT_FLAG_SINK))
    {
        RIALTO_SERVER_LOG_DEBUG("Store AutoAudioSink child sink");

        if (m_context.autoAudioChildSink && m_context.autoAudioChildSink != GST_ELEMENT(object))
        {
            RIALTO_SERVER_LOG_MIL("AutoAudioSink child is been overwritten");
        }
        m_context.autoAudioChildSink = GST_ELEMENT(object);
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

void GstGenericPlayer::removeAutoAudioSinkChild(GObject *object)
{
    if (GST_OBJECT_FLAG_IS_SET(GST_ELEMENT(object), GST_ELEMENT_FLAG_SINK))
    {
        RIALTO_SERVER_LOG_DEBUG("Remove AutoAudioSink child sink");

        if (m_context.autoAudioChildSink && m_context.autoAudioChildSink != GST_ELEMENT(object))
        {
            RIALTO_SERVER_LOG_MIL("AutoAudioSink child sink is not the same as the one stored");
            return;
        }

        m_context.autoAudioChildSink = nullptr;
    }
}

GstElement *GstGenericPlayer::getSinkChildIfAutoVideoSink(GstElement *sink) const
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
        }
        else
        {
            return m_context.autoVideoChildSink;
        }
    }
    return sink;
}

GstElement *GstGenericPlayer::getSinkChildIfAutoAudioSink(GstElement *sink) const
{
    const gchar *kTmpName = m_glibWrapper->gTypeName(G_OBJECT_TYPE(sink));
    if (!kTmpName)
        return sink;

    const std::string kElementTypeName{kTmpName};
    if (kElementTypeName == "GstAutoAudioSink")
    {
        if (!m_context.autoAudioChildSink)
        {
            RIALTO_SERVER_LOG_WARN("No child sink has been added to the autoaudiosink");
        }
        else
        {
            return m_context.autoAudioChildSink;
        }
    }
    return sink;
}

void GstGenericPlayer::setPlaybinFlags(bool enableAudio)
{
    unsigned flags = getGstPlayFlag("video") | getGstPlayFlag("native-video") | getGstPlayFlag("text");

    if (enableAudio)
    {
        flags |= getGstPlayFlag("audio");
        flags |= shouldEnableNativeAudio() ? getGstPlayFlag("native-audio") : 0;
    }

    m_glibWrapper->gObjectSet(m_context.pipeline, "flags", flags, nullptr);
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
