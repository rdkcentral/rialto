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
#include <cstring>
#include <ctime>
#include <stdexcept>

#include "FlushWatcher.h"
#include "GstDispatcherThread.h"
#include "GstGenericPlayer.h"
#include "GstProtectionMetadata.h"
#include "IGstTextTrackSinkFactory.h"
#include "IMediaPipeline.h"
#include "ITimer.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"
#include "Utils.h"
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
constexpr std::chrono::seconds kSubtitleClockResyncInterval{10};

bool operator==(const firebolt::rialto::server::SegmentData &lhs, const firebolt::rialto::server::SegmentData &rhs)
{
    return (lhs.position == rhs.position) && (lhs.resetTime == rhs.resetTime) && (lhs.appliedRate == rhs.appliedRate) &&
           (lhs.stopPosition == rhs.stopPosition);
}
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
                              rdkGstreamerUtilsWrapper, IGstInitialiser::instance(), std::make_unique<FlushWatcher>(),
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

GstGenericPlayer::GstGenericPlayer(
    IGstGenericPlayerClient *client, IDecryptionService &decryptionService, MediaType type,
    const VideoRequirements &videoRequirements,
    const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> &rdkGstreamerUtilsWrapper,
    const IGstInitialiser &gstInitialiser, std::unique_ptr<IFlushWatcher> &&flushWatcher,
    const std::shared_ptr<IGstSrcFactory> &gstSrcFactory, std::shared_ptr<common::ITimerFactory> timerFactory,
    std::unique_ptr<IGenericPlayerTaskFactory> taskFactory, std::unique_ptr<IWorkerThreadFactory> workerThreadFactory,
    std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory,
    std::shared_ptr<IGstProtectionMetadataHelperFactory> gstProtectionMetadataFactory)
    : m_gstPlayerClient(client), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper},
      m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper}, m_timerFactory{timerFactory},
      m_taskFactory{std::move(taskFactory)}, m_flushWatcher{std::move(flushWatcher)}
{
    RIALTO_SERVER_LOG_DEBUG("GstGenericPlayer is constructed.");

    gstInitialiser.waitForInitialisation();

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

    m_gstDispatcherThread = gstDispatcherThreadFactory->createGstDispatcherThread(*this, m_context.pipeline,
                                                                                  m_context.flushOnPrerollController,
                                                                                  m_gstWrapper);
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

    m_context.gstProfiler = std::make_unique<GstProfiler>(m_context.pipeline, m_gstWrapper, m_glibWrapper);

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
    if (GST_STATE_CHANGE_FAILURE == m_gstWrapper->gstElementSetState(m_context.pipeline, GST_STATE_READY))
    {
        GST_WARNING("Failed to set pipeline to READY state");
    }
    RIALTO_SERVER_LOG_MIL("New RialtoServer's pipeline created");
    auto recordId = m_context.gstProfiler->createRecord("Pipeline Created");
    if (recordId)
        m_context.gstProfiler->logRecord(recordId.value());
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
        m_context.subtitleSink = nullptr;
    }

    if (m_context.videoSink)
    {
        m_gstWrapper->gstObjectUnref(m_context.videoSink);
        m_context.videoSink = nullptr;
    }

    // Delete the pipeline
    m_gstWrapper->gstObjectUnref(m_context.pipeline);

    RIALTO_SERVER_LOG_MIL("RialtoServer's pipeline terminated");
    auto recordId = m_context.gstProfiler->createRecord("Pipeline Terminated");
    if (recordId)
        m_context.gstProfiler->logRecord(recordId.value());
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
    position = getPosition(m_context.pipeline);
    if (position == -1)
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
    case MediaSourceType::SUBTITLE:
        kSinkName = "text-sink";
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
        if (sink && firebolt::rialto::MediaSourceType::SUBTITLE != mediaSourceType)
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
    }
    return sink;
}

void GstGenericPlayer::setSourceFlushed(const MediaSourceType &mediaSourceType)
{
    m_flushWatcher->setFlushed(mediaSourceType);
}

void GstGenericPlayer::notifyPlaybackInfo()
{
    PlaybackInfo info;
    getPosition(info.currentPosition);
    getVolume(info.volume);
    m_gstPlayerClient->notifyPlaybackInfo(info);
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

std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate>
GstGenericPlayer::createAudioAttributes(const std::unique_ptr<IMediaPipeline::MediaSource> &source) const
{
    std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> audioAttributes;
    const IMediaPipeline::MediaSourceAudio *kSource = dynamic_cast<IMediaPipeline::MediaSourceAudio *>(source.get());
    if (kSource)
    {
        firebolt::rialto::AudioConfig audioConfig = kSource->getAudioConfig();
        audioAttributes =
            firebolt::rialto::wrappers::AudioAttributesPrivate{"", // param set below.
                                                               audioConfig.numberOfChannels, audioConfig.sampleRate,
                                                               0, // used only in one of logs in rdk_gstreamer_utils, no
                                                                  // need to set this param.
                                                               0, // used only in one of logs in rdk_gstreamer_utils, no
                                                                  // need to set this param.
                                                               audioConfig.codecSpecificConfig.data(),
                                                               static_cast<std::uint32_t>(
                                                                   audioConfig.codecSpecificConfig.size())};
        if (source->getMimeType() == "audio/mp4" || source->getMimeType() == "audio/aac")
        {
            audioAttributes->m_codecParam = "mp4a";
        }
        else if (source->getMimeType() == "audio/x-eac3")
        {
            audioAttributes->m_codecParam = "ec-3";
        }
        else if (source->getMimeType() == "audio/b-wav" || source->getMimeType() == "audio/x-raw")
        {
            audioAttributes->m_codecParam = "lpcm";
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to cast source");
    }

    return audioAttributes;
}

void GstGenericPlayer::configAudioCap(firebolt::rialto::wrappers::AudioAttributesPrivate *pAttrib, bool *audioaac,
                                      bool svpenabled, GstCaps **appsrcCaps)
{
    // this function comes from rdk_gstreamer_utils
    if (!pAttrib || !audioaac || !appsrcCaps)
    {
        RIALTO_SERVER_LOG_ERROR("configAudioCap: invalid null parameter");
        return;
    }
    gchar *capsString;
    RIALTO_SERVER_LOG_DEBUG("Config audio codec %s sampling rate %d channel %d alignment %d",
                            pAttrib->m_codecParam.c_str(), pAttrib->m_samplesPerSecond, pAttrib->m_numberOfChannels,
                            pAttrib->m_blockAlignment);
    if (pAttrib->m_codecParam.compare(0, 4, std::string("mp4a")) == 0)
    {
        RIALTO_SERVER_LOG_DEBUG("Using AAC");
        capsString = m_glibWrapper->gStrdupPrintf("audio/mpeg, mpegversion=4, enable-svp=(string)%s",
                                                  svpenabled ? "true" : "false");
        *audioaac = true;
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Using EAC3");
        capsString = m_glibWrapper->gStrdupPrintf("audio/x-eac3, framed=(boolean)true, rate=(int)%u, channels=(int)%u, "
                                                  "alignment=(string)frame, enable-svp=(string)%s",
                                                  pAttrib->m_samplesPerSecond, pAttrib->m_numberOfChannels,
                                                  svpenabled ? "true" : "false");
        *audioaac = false;
    }
    *appsrcCaps = m_gstWrapper->gstCapsFromString(capsString);
    m_glibWrapper->gFree(capsString);
}

void GstGenericPlayer::haltAudioPlayback()
{
    // this function comes from rdk_gstreamer_utils
    if (!m_context.playbackGroup.m_curAudioPlaysinkBin || !m_context.playbackGroup.m_curAudioDecodeBin)
    {
        RIALTO_SERVER_LOG_ERROR("haltAudioPlayback: audio playsink bin or decode bin is null");
        return;
    }
    GstState currentState{GST_STATE_VOID_PENDING}, pending{GST_STATE_VOID_PENDING};

    // Transition Playsink to Ready
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioPlaysinkBin, GST_STATE_READY))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set AudioPlaysinkBin to READY");
        return;
    }
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioPlaysinkBin, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    if (currentState == GST_STATE_PAUSED)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioPlaySinkBin State = %d", currentState);
    // Transition Decodebin to Paused
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioDecodeBin, GST_STATE_PAUSED))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set AudioDecodeBin to PAUSED");
        return;
    }
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioDecodeBin, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    if (currentState == GST_STATE_PAUSED)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current DecodeBin State = %d", currentState);
}

void GstGenericPlayer::resumeAudioPlayback()
{
    // this function comes from rdk_gstreamer_utils
    if (!m_context.playbackGroup.m_curAudioPlaysinkBin || !m_context.playbackGroup.m_curAudioDecodeBin)
    {
        RIALTO_SERVER_LOG_ERROR("resumeAudioPlayback: audio playsink bin or decode bin is null");
        return;
    }
    GstState currentState{GST_STATE_VOID_PENDING}, pending{GST_STATE_VOID_PENDING};
    m_gstWrapper->gstElementSyncStateWithParent(m_context.playbackGroup.m_curAudioPlaysinkBin);
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioPlaysinkBin, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> AudioPlaysinkbin State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstElementSyncStateWithParent(m_context.playbackGroup.m_curAudioDecodeBin);
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioDecodeBin, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> Decodebin State = %d Pending = %d", currentState, pending);
}

void GstGenericPlayer::firstTimeSwitchFromAC3toAAC(GstCaps *newAudioCaps)
{
    // this function comes from rdk_gstreamer_utils
    if (!m_context.playbackGroup.m_curAudioTypefind || !m_context.playbackGroup.m_curAudioDecodeBin)
    {
        RIALTO_SERVER_LOG_ERROR("firstTimeSwitchFromAC3toAAC: audio typefind or decode bin is null");
        return;
    }
    GstState currentState{GST_STATE_VOID_PENDING}, pending{GST_STATE_VOID_PENDING};
    GstPad *pTypfdSrcPad = NULL;
    GstPad *pTypfdSrcPeerPad = NULL;
    GstPad *pNewAudioDecoderSrcPad = NULL;
    GstElement *newAudioParse = NULL;
    GstElement *newAudioDecoder = NULL;
    GstElement *newQueue = NULL;
    gboolean linkRet = false;

    /* Get the SinkPad of ASink - pTypfdSrcPeerPad */
    if ((pTypfdSrcPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioTypefind, "src")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current Typefind SrcPad = %p", pTypfdSrcPad);
    if ((pTypfdSrcPeerPad = m_gstWrapper->gstPadGetPeer(pTypfdSrcPad)) != NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current Typefind Src Downstream Element Pad = %p", pTypfdSrcPeerPad);
    // AudioDecoder Downstream Unlink
    if (m_gstWrapper->gstPadUnlink(pTypfdSrcPad, pTypfdSrcPeerPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Downstream Unlink Failed");
    newAudioParse = m_gstWrapper->gstElementFactoryMake("aacparse", "aacparse");
    newAudioDecoder = m_gstWrapper->gstElementFactoryMake("avdec_aac", "avdec_aac");
    newQueue = m_gstWrapper->gstElementFactoryMake("queue", "aqueue");
    // Add new Decoder to Decodebin
    if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newAudioDecoder) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Added New AudioDecoder = %p", newAudioDecoder);
    }
    // Add new Parser to Decodebin
    if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newAudioParse) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Added New AudioParser = %p", newAudioParse);
    }
    // Add new Queue to Decodebin
    if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newQueue) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Added New queue = %p", newQueue);
    }
    if ((pNewAudioDecoderSrcPad = m_gstWrapper->gstElementGetStaticPad(newAudioDecoder, "src")) != NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Src Pad = %p", pNewAudioDecoderSrcPad);
    // Connect decoder to ASINK
    if (m_gstWrapper->gstPadLink(pNewAudioDecoderSrcPad, pTypfdSrcPeerPad) != GST_PAD_LINK_OK)
        RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Downstream Link Failed");
    linkRet = m_gstWrapper->gstElementLink(newAudioParse, newQueue) &&
              m_gstWrapper->gstElementLink(newQueue, newAudioDecoder);
    if (!linkRet)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Downstream Link Failed for typefind, parser, decoder");
    /* Force Caps */
    RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Setting to READY");
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioTypefind, GST_STATE_READY))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set Typefind to READY");
        m_gstWrapper->gstObjectUnref(pTypfdSrcPad);
        m_gstWrapper->gstObjectUnref(pTypfdSrcPeerPad);
        m_gstWrapper->gstObjectUnref(pNewAudioDecoderSrcPad);
        return;
    }
    m_glibWrapper->gObjectSet(G_OBJECT(m_context.playbackGroup.m_curAudioTypefind), "force-caps", newAudioCaps, NULL);
    m_gstWrapper->gstElementSyncStateWithParent(m_context.playbackGroup.m_curAudioTypefind);
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioTypefind, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New Typefind State = %d Pending = %d", currentState, pending);
    RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Syncing with Parent");
    m_context.playbackGroup.m_linkTypefindParser = true;
    /* Update the state */
    m_gstWrapper->gstElementSyncStateWithParent(newAudioDecoder);
    m_gstWrapper->gstElementGetState(newAudioDecoder, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstElementSyncStateWithParent(newQueue);
    m_gstWrapper->gstElementGetState(newQueue, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New queue State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstElementSyncStateWithParent(newAudioParse);
    m_gstWrapper->gstElementGetState(newAudioParse, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstObjectUnref(pTypfdSrcPad);
    m_gstWrapper->gstObjectUnref(pTypfdSrcPeerPad);
    m_gstWrapper->gstObjectUnref(pNewAudioDecoderSrcPad);
    return;
}

bool GstGenericPlayer::switchAudioCodec(bool isAudioAAC, GstCaps *newAudioCaps)
{ // this function comes from rdk_gstreamer_utils
    bool ret = false;
    RIALTO_SERVER_LOG_DEBUG("Current Audio Codec AAC = %d Same as Incoming audio Codec AAC = %d",
                            m_context.playbackGroup.m_isAudioAAC, isAudioAAC);
    if (m_context.playbackGroup.m_isAudioAAC == isAudioAAC)
    {
        return ret;
    }
    if ((m_context.playbackGroup.m_curAudioDecoder == NULL) && (!(m_context.playbackGroup.m_isAudioAAC)) && (isAudioAAC))
    {
        firstTimeSwitchFromAC3toAAC(newAudioCaps);
        m_context.playbackGroup.m_isAudioAAC = isAudioAAC;
        return true;
    }
    if (!m_context.playbackGroup.m_curAudioDecoder || !m_context.playbackGroup.m_curAudioParse ||
        !m_context.playbackGroup.m_curAudioDecodeBin)
    {
        RIALTO_SERVER_LOG_ERROR("switchAudioCodec: audio decoder, parser or decode bin is null");
        return false;
    }
    GstElement *newAudioParse = NULL;
    GstElement *newAudioDecoder = NULL;
    GstPad *newAudioParseSrcPad = NULL;
    GstPad *newAudioParseSinkPad = NULL;
    GstPad *newAudioDecoderSrcPad = NULL;
    GstPad *newAudioDecoderSinkPad = NULL;
    GstPad *audioDecSrcPad = NULL;
    GstPad *audioDecSinkPad = NULL;
    GstPad *audioDecSrcPeerPad = NULL;
    GstPad *audioDecSinkPeerPad = NULL;
    GstPad *audioParseSrcPad = NULL;
    GstPad *audioParseSinkPad = NULL;
    GstPad *audioParseSrcPeerPad = NULL;
    GstPad *audioParseSinkPeerPad = NULL;
    GstState currentState{GST_STATE_VOID_PENDING}, pending{GST_STATE_VOID_PENDING};

    // Get AudioDecoder Src Pads
    if ((audioDecSrcPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioDecoder, "src")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder Src Pad = %p", audioDecSrcPad);
    // Get AudioDecoder Sink Pads
    if ((audioDecSinkPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioDecoder, "sink")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder Sink Pad = %p", audioDecSinkPad);
    // Get AudioDecoder Src Peer i.e. Downstream Element Pad
    if ((audioDecSrcPeerPad = m_gstWrapper->gstPadGetPeer(audioDecSrcPad)) != NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder Src Downstream Element Pad = %p", audioDecSrcPeerPad);
    // Get AudioDecoder Sink Peer i.e. Upstream Element Pad
    if ((audioDecSinkPeerPad = m_gstWrapper->gstPadGetPeer(audioDecSinkPad)) != NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder Sink Upstream Element Pad = %p", audioDecSinkPeerPad);
    // Get AudioParser Src Pads
    if ((audioParseSrcPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioParse, "src")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser Src Pad = %p", audioParseSrcPad);
    // Get AudioParser Sink Pads
    if ((audioParseSinkPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioParse, "sink")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser Sink Pad = %p", audioParseSinkPad);
    // Get AudioParser Src Peer i.e. Downstream Element Pad
    if ((audioParseSrcPeerPad = m_gstWrapper->gstPadGetPeer(audioParseSrcPad)) != NULL) // Unref the Peer Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser Src Downstream Element Pad = %p", audioParseSrcPeerPad);
    // Get AudioParser Sink Peer i.e. Upstream Element Pad
    if ((audioParseSinkPeerPad = m_gstWrapper->gstPadGetPeer(audioParseSinkPad)) != NULL) // Unref the Peer Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser Sink Upstream Element Pad = %p", audioParseSinkPeerPad);
    // AudioDecoder Downstream Unlink
    if (m_gstWrapper->gstPadUnlink(audioDecSrcPad, audioDecSrcPeerPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> AudioDecoder Downstream Unlink Failed");
    // AudioDecoder Upstream Unlink
    if (m_gstWrapper->gstPadUnlink(audioDecSinkPeerPad, audioDecSinkPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> AudioDecoder Upstream Unlink Failed");
    // AudioParser Downstream Unlink
    if (m_gstWrapper->gstPadUnlink(audioParseSrcPad, audioParseSrcPeerPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> AudioParser Downstream Unlink Failed");
    // AudioParser Upstream Unlink
    if (m_gstWrapper->gstPadUnlink(audioParseSinkPeerPad, audioParseSinkPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> AudioParser Upstream Unlink Failed");
    // Current Audio Decoder NULL
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioDecoder, GST_STATE_NULL))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set AudioDecoder to NULL");
    }
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioDecoder, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    if (currentState == GST_STATE_NULL)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder State = %d", currentState);
    // Current Audio Parser NULL
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioParse, GST_STATE_NULL))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set AudioParser to NULL");
    }
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioParse, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    if (currentState == GST_STATE_NULL)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser State = %d", currentState);
    // Remove Audio Decoder From Decodebin
    if (m_gstWrapper->gstBinRemove(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()),
                                   m_context.playbackGroup.m_curAudioDecoder) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Removed AudioDecoder = %p", m_context.playbackGroup.m_curAudioDecoder);
        m_context.playbackGroup.m_curAudioDecoder = NULL;
    }
    // Remove Audio Parser From Decodebin
    if (m_gstWrapper->gstBinRemove(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()),
                                   m_context.playbackGroup.m_curAudioParse) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Removed AudioParser = %p", m_context.playbackGroup.m_curAudioParse);
        m_context.playbackGroup.m_curAudioParse = NULL;
    }
    // Create new Audio Decoder and Parser. The inverse of the current
    if (m_context.playbackGroup.m_isAudioAAC)
    {
        newAudioParse = m_gstWrapper->gstElementFactoryMake("ac3parse", "ac3parse");
        newAudioDecoder = m_gstWrapper->gstElementFactoryMake("identity", "fake_aud_ac3dec");
    }
    else
    {
        newAudioParse = m_gstWrapper->gstElementFactoryMake("aacparse", "aacparse");
        newAudioDecoder = m_gstWrapper->gstElementFactoryMake("avdec_aac", "avdec_aac");
    }
    {
        GstPadLinkReturn gstPadLinkRet = GST_PAD_LINK_OK;
        GstElement *audioParseUpstreamEl = NULL;
        // Add new Decoder to Decodebin
        if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newAudioDecoder) == TRUE)
        {
            RIALTO_SERVER_LOG_DEBUG("OTF -> Added New AudioDecoder = %p", newAudioDecoder);
        }
        // Add new Parser to Decodebin
        if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newAudioParse) == TRUE)
        {
            RIALTO_SERVER_LOG_DEBUG("OTF -> Added New AudioParser = %p", newAudioParse);
        }
        if ((newAudioDecoderSrcPad = m_gstWrapper->gstElementGetStaticPad(newAudioDecoder, "src")) !=
            NULL) // Unref the Pad
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Src Pad = %p", newAudioDecoderSrcPad);
        if ((newAudioDecoderSinkPad = m_gstWrapper->gstElementGetStaticPad(newAudioDecoder, "sink")) !=
            NULL) // Unref the Pad
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Sink Pad = %p", newAudioDecoderSinkPad);
        // Link New Decoder to Downstream followed by UpStream
        if ((gstPadLinkRet = m_gstWrapper->gstPadLink(newAudioDecoderSrcPad, audioDecSrcPeerPad)) != GST_PAD_LINK_OK)
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Downstream Link Failed");
        if ((gstPadLinkRet = m_gstWrapper->gstPadLink(audioDecSinkPeerPad, newAudioDecoderSinkPad)) != GST_PAD_LINK_OK)
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Upstream Link Failed");
        if ((newAudioParseSrcPad = m_gstWrapper->gstElementGetStaticPad(newAudioParse, "src")) != NULL) // Unref the Pad
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser Src Pad = %p", newAudioParseSrcPad);
        if ((newAudioParseSinkPad = m_gstWrapper->gstElementGetStaticPad(newAudioParse, "sink")) != NULL) // Unref the Pad
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser Sink Pad = %p", newAudioParseSinkPad);
        // Link New Parser to Downstream followed by UpStream
        if ((gstPadLinkRet = m_gstWrapper->gstPadLink(newAudioParseSrcPad, audioParseSrcPeerPad)) != GST_PAD_LINK_OK)
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser Downstream Link Failed %d", gstPadLinkRet);
        if ((audioParseUpstreamEl = GST_ELEMENT_CAST(m_gstWrapper->gstPadGetParent(audioParseSinkPeerPad))) ==
            m_context.playbackGroup.m_curAudioTypefind)
        {
            RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Setting to READY");
            if (GST_STATE_CHANGE_FAILURE == m_gstWrapper->gstElementSetState(audioParseUpstreamEl, GST_STATE_READY))
            {
                RIALTO_SERVER_LOG_WARN("Failed to set Typefind to READY in switchAudioCodec");
            }
            m_glibWrapper->gObjectSet(G_OBJECT(audioParseUpstreamEl), "force-caps", newAudioCaps, NULL);
            m_gstWrapper->gstElementSyncStateWithParent(audioParseUpstreamEl);
            m_gstWrapper->gstElementGetState(audioParseUpstreamEl, &currentState, &pending, GST_CLOCK_TIME_NONE);
            RIALTO_SERVER_LOG_DEBUG("OTF -> New Typefind State = %d Pending = %d", currentState, pending);
            RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Syncing with Parent");
            m_context.playbackGroup.m_linkTypefindParser = true;
            m_gstWrapper->gstObjectUnref(audioParseUpstreamEl);
        }
        m_gstWrapper->gstObjectUnref(newAudioDecoderSrcPad);
        m_gstWrapper->gstObjectUnref(newAudioDecoderSinkPad);
        m_gstWrapper->gstObjectUnref(newAudioParseSrcPad);
        m_gstWrapper->gstObjectUnref(newAudioParseSinkPad);
    }
    m_gstWrapper->gstObjectUnref(audioParseSinkPeerPad);
    m_gstWrapper->gstObjectUnref(audioParseSrcPeerPad);
    m_gstWrapper->gstObjectUnref(audioParseSinkPad);
    m_gstWrapper->gstObjectUnref(audioParseSrcPad);
    m_gstWrapper->gstObjectUnref(audioDecSinkPeerPad);
    m_gstWrapper->gstObjectUnref(audioDecSrcPeerPad);
    m_gstWrapper->gstObjectUnref(audioDecSinkPad);
    m_gstWrapper->gstObjectUnref(audioDecSrcPad);
    m_gstWrapper->gstElementSyncStateWithParent(newAudioDecoder);
    m_gstWrapper->gstElementGetState(newAudioDecoder, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstElementSyncStateWithParent(newAudioParse);
    m_gstWrapper->gstElementGetState(newAudioParse, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser State = %d Pending = %d", currentState, pending);
    m_context.playbackGroup.m_isAudioAAC = isAudioAAC;
    return true;
}

bool GstGenericPlayer::performAudioTrackCodecChannelSwitch(const void *pSampleAttr,
                                                           firebolt::rialto::wrappers::AudioAttributesPrivate *pAudioAttr,
                                                           uint32_t *pStatus, unsigned int *pui32Delay,
                                                           long long *pAudioChangeTargetPts, // NOLINT(runtime/int)
                                                           const long long *pcurrentDispPts, // NOLINT(runtime/int)
                                                           unsigned int *audioChangeStage, GstCaps **appsrcCaps,
                                                           bool *audioaac, bool svpenabled, GstElement *aSrc, bool *ret)
{
    // this function comes from rdk_gstreamer_utils
    if (!pStatus || !pui32Delay || !pAudioChangeTargetPts || !pcurrentDispPts || !audioChangeStage || !appsrcCaps ||
        !audioaac || !aSrc || !ret)
    {
        RIALTO_SERVER_LOG_ERROR("performAudioTrackCodecChannelSwitch: invalid null parameter");
        return false;
    }

    constexpr uint32_t kOk = 0;
    constexpr uint32_t kWaitWhileIdling = 100;
    constexpr int kAudioChangeGapThresholdMS = 40;
    constexpr unsigned int kAudchgAlign = 3;

    struct timespec ts, now;
    unsigned int reconfigDelayMs;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (*pStatus != kOk || pSampleAttr == nullptr)
    {
        RIALTO_SERVER_LOG_DEBUG("No audio data ready yet");
        *pui32Delay = kWaitWhileIdling;
        *ret = false;
        return true;
    }
    RIALTO_SERVER_LOG_DEBUG("Received first audio packet after a flush, PTS");
    if (pAudioAttr)
    {
        const char *pCodecStr = pAudioAttr->m_codecParam.c_str();
        const char *pCodecAcc = strstr(pCodecStr, "mp4a");
        bool isAudioAAC = (pCodecAcc) ? true : false;
        bool isCodecSwitch = false;
        RIALTO_SERVER_LOG_DEBUG("Audio Attribute format %s channel %d samp %d, bitrate %d blockAlignment %d", pCodecStr,
                                pAudioAttr->m_numberOfChannels, pAudioAttr->m_samplesPerSecond, pAudioAttr->m_bitrate,
                                pAudioAttr->m_blockAlignment);
        *pAudioChangeTargetPts = *pcurrentDispPts;
        *audioChangeStage = kAudchgAlign;
        if (*appsrcCaps)
        {
            m_gstWrapper->gstCapsUnref(*appsrcCaps);
            *appsrcCaps = NULL;
        }
        if (isAudioAAC != *audioaac)
            isCodecSwitch = true;
        configAudioCap(pAudioAttr, audioaac, svpenabled, appsrcCaps);
        {
            gboolean sendRet = FALSE;
            GstEvent *flushStart = NULL;
            GstEvent *flushStop = NULL;
            flushStart = m_gstWrapper->gstEventNewFlushStart();
            sendRet = m_gstWrapper->gstElementSendEvent(aSrc, flushStart);
            if (!sendRet)
                RIALTO_SERVER_LOG_DEBUG("failed to send flush-start event");
            flushStop = m_gstWrapper->gstEventNewFlushStop(TRUE);
            sendRet = m_gstWrapper->gstElementSendEvent(aSrc, flushStop);
            if (!sendRet)
                RIALTO_SERVER_LOG_DEBUG("failed to send flush-stop event");
        }
        if (!isCodecSwitch)
        {
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(aSrc), *appsrcCaps);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("CODEC SWITCH mAudioAAC = %d", *audioaac);
            haltAudioPlayback();
            if (switchAudioCodec(*audioaac, *appsrcCaps) == false)
            {
                RIALTO_SERVER_LOG_DEBUG("CODEC SWITCH FAILED switchAudioCodec mAudioAAC = %d", *audioaac);
            }
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(aSrc), *appsrcCaps);
            resumeAudioPlayback();
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        reconfigDelayMs = now.tv_nsec > ts.tv_nsec ? (now.tv_nsec - ts.tv_nsec) / 1000000
                                                   : (1000 - (ts.tv_nsec - now.tv_nsec) / 1000000);
        (*pAudioChangeTargetPts) += (reconfigDelayMs + kAudioChangeGapThresholdMS);
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("first audio after change no attribute drop!");
        *pui32Delay = 0;
        *ret = false;
        return true;
    }
    *ret = true;
    return true;
}

bool GstGenericPlayer::setImmediateOutput(const MediaSourceType &mediaSourceType, bool immediateOutputParam)
{
    if (!m_workerThread)
        return false;

    m_workerThread->enqueueTask(
        m_taskFactory->createSetImmediateOutput(m_context, *this, mediaSourceType, immediateOutputParam));
    return true;
}

bool GstGenericPlayer::setReportDecodeErrors(const MediaSourceType &mediaSourceType, bool reportDecodeErrors)
{
    if (!m_workerThread)
        return false;

    m_workerThread->enqueueTask(
        m_taskFactory->createSetReportDecodeErrors(m_context, *this, mediaSourceType, reportDecodeErrors));
    return true;
}

bool GstGenericPlayer::getQueuedFrames(uint32_t &queuedFrames)
{
    bool returnValue{false};
    GstElement *decoder{getDecoder(MediaSourceType::VIDEO)};
    if (decoder)
    {
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "queued-frames"))
        {
            m_glibWrapper->gObjectGet(decoder, "queued-frames", &queuedFrames, nullptr);
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("queued-frames not supported in element %s", GST_ELEMENT_NAME(decoder));
        }
        m_gstWrapper->gstObjectUnref(decoder);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get queued-frames property, decoder is NULL");
    }

    return returnValue;
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
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get stats, sink is NULL");
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

        if (firebolt::rialto::MediaSourceType::SUBTITLE == mediaType)
        {
            setTextTrackPositionIfRequired(streamInfo.appSrc);
        }
        else
        {
            pushSampleIfRequired(streamInfo.appSrc, common::convertMediaSourceType(mediaType));
        }
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
            RIALTO_SERVER_LOG_MIL("Buffered NetworkState reached");
        }
        cancelUnderflow(mediaType);

        const auto eosInfoIt = m_context.endOfStreamInfo.find(mediaType);
        if (eosInfoIt != m_context.endOfStreamInfo.end() && eosInfoIt->second == EosState::PENDING)
        {
            setEos(mediaType);
        }
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
    // GstAppSrc does not replace segment, if it's the same as previous one.
    // It causes problems with position reporing in amlogic devices, so we need to push
    // two segments with different reset time value.
    pushAdditionalSegmentIfRequired(source);

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
        auto recordId = m_context.gstProfiler->createRecord("First Segment Received", typeStr);
        if (recordId)
            m_context.gstProfiler->logRecord(recordId.value());

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
    m_context.currentPosition[source] = initialPosition->second.back();
    m_context.initialPositions.erase(initialPosition);
    return;
}

void GstGenericPlayer::pushAdditionalSegmentIfRequired(GstElement *source)
{
    auto currentPosition = m_context.currentPosition.find(source);
    if (m_context.currentPosition.end() == currentPosition)
    {
        return;
    }
    auto initialPosition = m_context.initialPositions.find(source);
    if (m_context.initialPositions.end() == initialPosition)
    {
        return;
    }
    if (initialPosition->second.size() == 1 && initialPosition->second.back().resetTime &&
        currentPosition->second == initialPosition->second.back())
    {
        RIALTO_SERVER_LOG_INFO("Adding additional segment with reset_time = false");
        SegmentData additionalSegment = initialPosition->second.back();
        additionalSegment.resetTime = false;
        initialPosition->second.push_back(additionalSegment);
    }
}

void GstGenericPlayer::setTextTrackPositionIfRequired(GstElement *source)
{
    auto initialPosition = m_context.initialPositions.find(source);
    if (m_context.initialPositions.end() == initialPosition)
    {
        // Sending initial sample not needed
        return;
    }

    RIALTO_SERVER_LOG_MIL("New subtitle position set %" GST_TIME_FORMAT,
                          GST_TIME_ARGS(initialPosition->second.back().position));
    m_glibWrapper->gObjectSet(m_context.subtitleSink, "position",
                              static_cast<guint64>(initialPosition->second.back().position), nullptr);

    m_context.initialPositions.erase(initialPosition);
}

bool GstGenericPlayer::reattachSource(const std::unique_ptr<IMediaPipeline::MediaSource> &source)
{
    if (m_context.streamInfo.find(source->getType()) == m_context.streamInfo.end())
    {
        RIALTO_SERVER_LOG_ERROR("Unable to switch source, type does not exist");
        return false;
    }
    if (source->getMimeType().empty())
    {
        RIALTO_SERVER_LOG_WARN("Skip switch audio source. Unknown mime type");
        return false;
    }
    std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> audioAttributes{createAudioAttributes(source)};
    if (!audioAttributes)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create audio attributes");
        return false;
    }

    long long currentDispPts = getPosition(m_context.pipeline); // NOLINT(runtime/int)
    GstCaps *caps{createCapsFromMediaSource(m_gstWrapper, m_glibWrapper, source)};
    GstAppSrc *appSrc{GST_APP_SRC(m_context.streamInfo[source->getType()].appSrc)};
    GstCaps *oldCaps = m_gstWrapper->gstAppSrcGetCaps(appSrc);

    if ((!oldCaps) || (!m_gstWrapper->gstCapsIsEqual(caps, oldCaps)))
    {
        RIALTO_SERVER_LOG_DEBUG("Caps not equal. Perform audio track codec channel switch.");

        GstElement *sink = getSink(MediaSourceType::AUDIO);
        if (!sink)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to get audio sink");
            if (caps)
                m_gstWrapper->gstCapsUnref(caps);
            if (oldCaps)
                m_gstWrapper->gstCapsUnref(oldCaps);
            return false;
        }
        std::string sinkName = GST_ELEMENT_NAME(sink);
        m_gstWrapper->gstObjectUnref(sink);

        int sampleAttributes{
            0}; // rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch checks if this param != NULL only.
        std::uint32_t status{0};   // must be 0 to make rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch work
        unsigned int ui32Delay{0}; // output param
        long long audioChangeTargetPts{-1}; // NOLINT(runtime/int) output param. Set audioChangeTargetPts =
                                            // currentDispPts in rdk_gstreamer_utils function stub
        unsigned int audioChangeStage{0};   // Output param. Set to AUDCHG_ALIGN in rdk_gstreamer_utils function stub
        gchar *oldCapsCStr = m_gstWrapper->gstCapsToString(oldCaps);
        std::string oldCapsStr = std::string(oldCapsCStr);
        m_glibWrapper->gFree(oldCapsCStr);
        bool audioAac{oldCapsStr.find("audio/mpeg") != std::string::npos};
        bool svpEnabled{true}; // assume always true
        bool retVal{false};    // Output param. Set to TRUE in rdk_gstreamer_utils function stub

        bool result = false;
        if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "amlhalasink"))
        {
            // due to problems audio codec change in prerolling, temporarily moved the code from rdk gstreamer utils to
            // Rialto and applied fixes
            result = performAudioTrackCodecChannelSwitch(&sampleAttributes, &(*audioAttributes), &status, &ui32Delay,
                                                         &audioChangeTargetPts, &currentDispPts, &audioChangeStage,
                                                         &caps, &audioAac, svpEnabled, GST_ELEMENT(appSrc), &retVal);
        }
        else
        {
            result = m_rdkGstreamerUtilsWrapper->performAudioTrackCodecChannelSwitch(&m_context.playbackGroup,
                                                                                     &sampleAttributes,
                                                                                     &(*audioAttributes), &status,
                                                                                     &ui32Delay, &audioChangeTargetPts,
                                                                                     &currentDispPts, &audioChangeStage,
                                                                                     &caps, &audioAac, svpEnabled,
                                                                                     GST_ELEMENT(appSrc), &retVal);
        }

        if (!result || !retVal)
        {
            RIALTO_SERVER_LOG_WARN("performAudioTrackCodecChannelSwitch failed! Result: %d, retval %d", result, retVal);
        }
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Skip switching audio source - caps are the same.");
    }

    m_context.lastAudioSampleTimestamps = currentDispPts;
    if (caps)
        m_gstWrapper->gstCapsUnref(caps);
    if (oldCaps)
        m_gstWrapper->gstCapsUnref(oldCaps);

    return true;
}

bool GstGenericPlayer::hasSourceType(const MediaSourceType &mediaSourceType) const
{
    return m_context.streamInfo.find(mediaSourceType) != m_context.streamInfo.end();
}

void GstGenericPlayer::scheduleNeedMediaData(GstAppSrc *src)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createNeedData(m_context, *this, src));
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
        bool underflowEnabled = m_context.isPlaying;
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

void GstGenericPlayer::play(bool &async)
{
    if (0 == m_ongoingStateChangesNumber)
    {
        // Operation called on main thread, because PAUSED->PLAYING change is synchronous and needs to be done fast.
        //
        // m_context.pipeline can be used, because it's modified only in GstGenericPlayer
        // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
        ++m_ongoingStateChangesNumber;
        async = (changePipelineState(GST_STATE_PLAYING) == GST_STATE_CHANGE_ASYNC);
        RIALTO_SERVER_LOG_MIL("State change to PLAYING requested");
    }
    else
    {
        ++m_ongoingStateChangesNumber;
        async = true;
        if (m_workerThread)
        {
            m_workerThread->enqueueTask(m_taskFactory->createPlay(*this));
        }
    }
}

void GstGenericPlayer::pause()
{
    ++m_ongoingStateChangesNumber;
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createPause(m_context, *this));
    }
}

void GstGenericPlayer::stop()
{
    ++m_ongoingStateChangesNumber;
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createStop(m_context, *this));
    }
}

GstStateChangeReturn GstGenericPlayer::changePipelineState(GstState newState)
{
    if (!m_context.pipeline)
    {
        RIALTO_SERVER_LOG_ERROR("Change state failed - pipeline is nullptr");
        if (m_gstPlayerClient)
            m_gstPlayerClient->notifyPlaybackState(PlaybackState::FAILURE);
        --m_ongoingStateChangesNumber;
        return GST_STATE_CHANGE_FAILURE;
    }
    m_context.flushOnPrerollController->setTargetState(newState);
    const GstStateChangeReturn result{m_gstWrapper->gstElementSetState(m_context.pipeline, newState)};
    if (result == GST_STATE_CHANGE_FAILURE)
    {
        RIALTO_SERVER_LOG_ERROR("Change state failed - Gstreamer returned an error");
        if (m_gstPlayerClient)
            m_gstPlayerClient->notifyPlaybackState(PlaybackState::FAILURE);
    }
    --m_ongoingStateChangesNumber;
    return result;
}

int64_t GstGenericPlayer::getPosition(GstElement *element)
{
    if (!element)
    {
        RIALTO_SERVER_LOG_WARN("Element is null");
        return -1;
    }

    m_gstWrapper->gstStateLock(element);

    if (m_gstWrapper->gstElementGetState(element) < GST_STATE_PAUSED ||
        (m_gstWrapper->gstElementGetStateReturn(element) == GST_STATE_CHANGE_ASYNC &&
         m_gstWrapper->gstElementGetStateNext(element) == GST_STATE_PAUSED))
    {
        RIALTO_SERVER_LOG_WARN("Element is prerolling or in invalid state - state: %s, return: %s, next: %s",
                               m_gstWrapper->gstElementStateGetName(m_gstWrapper->gstElementGetState(element)),
                               m_gstWrapper->gstElementStateChangeReturnGetName(
                                   m_gstWrapper->gstElementGetStateReturn(element)),
                               m_gstWrapper->gstElementStateGetName(m_gstWrapper->gstElementGetStateNext(element)));

        m_gstWrapper->gstStateUnlock(element);
        return -1;
    }
    m_gstWrapper->gstStateUnlock(element);

    gint64 position = -1;
    if (!m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &position))
    {
        RIALTO_SERVER_LOG_WARN("Failed to query position");
        return -1;
    }

    return position;
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
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set video rectangle, sink is NULL");
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

bool GstGenericPlayer::setReportDecodeErrors()
{
    bool result{false};
    bool reportDecodeErrors{false};

    {
        std::unique_lock lock{m_context.propertyMutex};
        if (!m_context.pendingReportDecodeErrorsForVideo.has_value())
        {
            return false;
        }
        reportDecodeErrors = m_context.pendingReportDecodeErrorsForVideo.value();
    }

    GstElement *decoder = getDecoder(MediaSourceType::VIDEO);
    if (decoder)
    {
        RIALTO_SERVER_LOG_DEBUG("Set report decode errors to %s", reportDecodeErrors ? "TRUE" : "FALSE");

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "report-decode-errors"))
        {
            gboolean reportDecodeErrorsGboolean{reportDecodeErrors ? TRUE : FALSE};
            m_glibWrapper->gObjectSet(decoder, "report-decode-errors", reportDecodeErrorsGboolean, nullptr);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set report-decode-errors property on decoder '%s'",
                                    GST_ELEMENT_NAME(decoder));
        }

        m_gstWrapper->gstObjectUnref(decoder);

        {
            std::unique_lock lock{m_context.propertyMutex};
            m_context.pendingReportDecodeErrorsForVideo.reset();
        }
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Pending report-decode-errors, decoder is NULL");
    }
    return result;
}

bool GstGenericPlayer::setShowVideoWindow()
{
    if (!m_context.pendingShowVideoWindow.has_value())
    {
        RIALTO_SERVER_LOG_WARN("No show video window value to be set. Aborting...");
        return false;
    }

    GstElement *videoSink{getSink(MediaSourceType::VIDEO)};
    if (!videoSink)
    {
        RIALTO_SERVER_LOG_DEBUG("Setting show video window queued. Video sink is NULL");
        return false;
    }
    bool result{false};
    if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), "show-video-window"))
    {
        m_glibWrapper->gObjectSet(videoSink, "show-video-window", m_context.pendingShowVideoWindow.value(), nullptr);
        result = true;
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Setting show video window failed. Property does not exist");
    }
    m_context.pendingShowVideoWindow.reset();
    m_gstWrapper->gstObjectUnref(GST_OBJECT(videoSink));
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
                m_workerThread->enqueueTask(m_taskFactory->createReportPosition(m_context, *this));
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

void GstGenericPlayer::startNotifyPlaybackInfoTimer()
{
    static constexpr std::chrono::milliseconds kPlaybackInfoTimerMs{32};
    if (m_playbackInfoTimer && m_playbackInfoTimer->isActive())
    {
        return;
    }

    notifyPlaybackInfo();

    m_playbackInfoTimer =
        m_timerFactory
            ->createTimer(kPlaybackInfoTimerMs, [this]() { notifyPlaybackInfo(); }, firebolt::rialto::common::TimerType::PERIODIC);
}

void GstGenericPlayer::stopNotifyPlaybackInfoTimer()
{
    if (m_playbackInfoTimer && m_playbackInfoTimer->isActive())
    {
        m_playbackInfoTimer->cancel();
        m_playbackInfoTimer.reset();
    }
}

void GstGenericPlayer::startSubtitleClockResyncTimer()
{
    if (m_subtitleClockResyncTimer && m_subtitleClockResyncTimer->isActive())
    {
        return;
    }

    m_subtitleClockResyncTimer = m_timerFactory->createTimer(
        kSubtitleClockResyncInterval,
        [this]()
        {
            if (m_workerThread)
            {
                m_workerThread->enqueueTask(m_taskFactory->createSynchroniseSubtitleClock(m_context, *this));
            }
        },
        firebolt::rialto::common::TimerType::PERIODIC);
}

void GstGenericPlayer::stopSubtitleClockResyncTimer()
{
    if (m_subtitleClockResyncTimer && m_subtitleClockResyncTimer->isActive())
    {
        m_subtitleClockResyncTimer->cancel();
        m_subtitleClockResyncTimer.reset();
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

bool GstGenericPlayer::getVolume(double &currentVolume)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstGenericPlayer
    // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
    if (!m_context.pipeline)
    {
        return false;
    }

    // NOTE: No gstreamer documentation for "fade-volume" could be found at the time this code was written.
    // Therefore the author performed several tests on a supported platform (Flex2) to determine the behaviour of this property.
    // The code has been written to be backwardly compatible on platforms that don't have this property.
    // The observed behaviour was:
    //    - if the returned fade volume is negative then audio-fade is not active. In this case the usual technique
    //      to find volume in the pipeline works and is used.
    //    - if the returned fade volume is positive then audio-fade is active. In this case the returned fade volume
    //      directly returns the current volume level 0=min to 100=max (and the pipeline's current volume level is
    //      meaningless and doesn't contribute in this case).
    GstElement *sink{getSink(MediaSourceType::AUDIO)};
    if (m_context.audioFadeEnabled && sink &&
        m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "fade-volume"))
    {
        gint fadeVolume{-100};
        m_glibWrapper->gObjectGet(sink, "fade-volume", &fadeVolume, NULL);
        if (fadeVolume < 0)
        {
            currentVolume = m_gstWrapper->gstStreamVolumeGetVolume(GST_STREAM_VOLUME(m_context.pipeline),
                                                                   GST_STREAM_VOLUME_FORMAT_LINEAR);
            RIALTO_SERVER_LOG_INFO("Fade volume is negative, using volume from pipeline: %f", currentVolume);
        }
        else
        {
            currentVolume = static_cast<double>(fadeVolume) / 100.0;
            RIALTO_SERVER_LOG_INFO("Fade volume is supported: %f", currentVolume);
        }
    }
    else
    {
        currentVolume = m_gstWrapper->gstStreamVolumeGetVolume(GST_STREAM_VOLUME(m_context.pipeline),
                                                               GST_STREAM_VOLUME_FORMAT_LINEAR);
        RIALTO_SERVER_LOG_INFO("Fade volume is not supported, using volume from pipeline: %f", currentVolume);
    }

    if (sink)
        m_gstWrapper->gstObjectUnref(sink);

    return true;
}

void GstGenericPlayer::setMute(const MediaSourceType &mediaSourceType, bool mute)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetMute(m_context, *this, mediaSourceType, mute));
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

bool GstGenericPlayer::isAsync(const MediaSourceType &mediaSourceType) const
{
    GstElement *sink = getSink(mediaSourceType);
    if (!sink)
    {
        RIALTO_SERVER_LOG_WARN("Sink not found for %s", common::convertMediaSourceType(mediaSourceType));
        return true; // Our sinks are async by default
    }
    gboolean returnValue{TRUE};
    m_glibWrapper->gObjectGet(sink, "async", &returnValue, nullptr);
    m_gstWrapper->gstObjectUnref(sink);
    return returnValue == TRUE;
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

void GstGenericPlayer::flush(const MediaSourceType &mediaSourceType, bool resetTime, bool &async)
{
    if (m_workerThread)
    {
        async = isAsync(mediaSourceType);
        m_flushWatcher->setFlushing(mediaSourceType, async);
        m_workerThread->enqueueTask(m_taskFactory->createFlush(m_context, *this, mediaSourceType, resetTime, async));
    }
}

void GstGenericPlayer::setSourcePosition(const MediaSourceType &mediaSourceType, int64_t position, bool resetTime,
                                         double appliedRate, uint64_t stopPosition)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSourcePosition(m_context, mediaSourceType, position,
                                                                           resetTime, appliedRate, stopPosition));
    }
}

void GstGenericPlayer::setSubtitleOffset(int64_t position)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSubtitleOffset(m_context, position));
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

void GstGenericPlayer::switchSource(const std::unique_ptr<IMediaPipeline::MediaSource> &mediaSource)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSwitchSource(*this, mediaSource));
    }
}

void GstGenericPlayer::handleBusMessage(GstMessage *message)
{
    m_workerThread->enqueueTask(m_taskFactory->createHandleBusMessage(m_context, *this, message, *m_flushWatcher));
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
