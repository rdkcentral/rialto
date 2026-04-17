/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "GstProfiler.h"
#include "RialtoCommonLogging.h"
#include "Utils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

#include <glib.h>
#include <gst/gst.h>

namespace firebolt::rialto::server
{
std::weak_ptr<IGstProfilerFactory> GstProfilerFactory::m_factory;

std::shared_ptr<IGstProfilerFactory> IGstProfilerFactory::getFactory()
{
    std::shared_ptr<IGstProfilerFactory> factory = GstProfilerFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<GstProfilerFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_COMMON_LOG_ERROR("Failed to create the gst profiler factory, reason: %s", e.what());
        }

        GstProfilerFactory::m_factory = factory;
    }

    return factory;
}

std::unique_ptr<IGstProfiler> GstProfilerFactory::createGstProfiler(GstElement *pipeline,
                                                                    const std::shared_ptr<IGstWrapper> &gstWrapper,
                                                                    const std::shared_ptr<IGlibWrapper> &glibWrapper) const
{
    return std::make_unique<GstProfiler>(pipeline, gstWrapper, glibWrapper);
}

inline constexpr std::array kKlassTokens{
    std::string_view{"Source"},
    std::string_view{"Decryptor"},
    std::string_view{"Decoder"},
};

GstProfiler::GstProfiler(GstElement *pipeline, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                         const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper)
    : m_pipeline{pipeline}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}
{
    auto profilerFactory = firebolt::rialto::common::IProfilerFactory::createFactory();
    m_profiler = profilerFactory ? profilerFactory->createProfiler(std::string{k_module}) : nullptr;
    m_enabled = (m_profiler != nullptr) && m_profiler->isEnabled();

    if (m_enabled && m_pipeline)
        m_gstWrapper->gstObjectRef(m_pipeline);
}

GstProfiler::~GstProfiler()
{
    if (m_enabled && m_pipeline)
        m_gstWrapper->gstObjectUnref(m_pipeline);
}

bool GstProfiler::isEnabled() const
{
    return m_enabled;
}

std::optional<GstProfiler::RecordId> GstProfiler::createRecord(std::string stage)
{
    if (!m_enabled || !m_profiler)
        return std::nullopt;

    auto id = m_profiler->record(std::move(stage));
    if (!id)
        return std::nullopt;

    return static_cast<GstProfiler::RecordId>(*id);
}

std::optional<GstProfiler::RecordId> GstProfiler::createRecord(std::string stage, std::string info)
{
    if (!m_enabled || !m_profiler)
        return std::nullopt;

    auto id = m_profiler->record(std::move(stage), std::move(info));
    if (!id)
        return std::nullopt;

    return static_cast<GstProfiler::RecordId>(*id);
}

void GstProfiler::scheduleGstElementRecord(GstElement *element)
{
    if (!m_enabled || !m_profiler)
        return;

    if (!element)
        return;

    auto stage = checkElement(element);
    if (!stage)
        return;

    GstPad *pad = m_gstWrapper->gstElementGetStaticPad(element, "src");
    if (!pad)
        return;

    std::string elementInfo;
    if (isVideo(*m_gstWrapper, element))
    {
        elementInfo = "Video";
    }
    else if (isAudio(*m_gstWrapper, element))
    {
        elementInfo = "Audio";
    }
    else
    {
        gchar *rawName = m_gstWrapper->gstElementGetName(element);
        elementInfo = rawName ? rawName : "<null>";
        if (rawName)
            m_glibWrapper->gFree(rawName);
    }

    auto *probeCtx = new ProbeCtx{this, stage.value(), std::move(elementInfo)};
    m_gstWrapper->gstPadAddProbe(pad, GST_PAD_PROBE_TYPE_BUFFER, &GstProfiler::probeCb, probeCtx,
                                 &GstProfiler::probeCtxDestroy);

    m_gstWrapper->gstObjectUnref(pad);
}

void GstProfiler::logRecord(GstProfiler::RecordId id)
{
    if (!m_enabled || !m_profiler)
        return;

    m_profiler->log(static_cast<firebolt::rialto::common::IProfiler::RecordId>(id));
}

void GstProfiler::dumpToFile() const
{
    if (!m_enabled || !m_profiler)
        return;

    (void)m_profiler->dumpToFile();
}

void GstProfiler::logPipeline() const
{
    if (!m_enabled || !m_profiler)
        return;

    const auto metrics = calculateMetrics();
    if (metrics)
    {
        RIALTO_COMMON_LOG_MIL("PROFILER | TUNETIME: %lld,  %lld,  %lld,  %lld,  %lld,  %lld,  %lld,  %lld,  %lld,  "
                              "%lld,  %lld,  %lld,  %lld",
                              metrics->preparation ? static_cast<long long>(*metrics->preparation) : -1,     // NOLINT
                              metrics->videoDownload ? static_cast<long long>(*metrics->videoDownload) : -1, // NOLINT
                              metrics->audioDownload ? static_cast<long long>(*metrics->audioDownload) : -1, // NOLINT
                              metrics->videoSource ? static_cast<long long>(*metrics->videoSource) : -1,     // NOLINT
                              metrics->audioSource ? static_cast<long long>(*metrics->audioSource) : -1,     // NOLINT
                              metrics->videoDecryption ? static_cast<long long>(*metrics->videoDecryption) : -1, // NOLINT
                              metrics->audioDecryption ? static_cast<long long>(*metrics->audioDecryption) : -1, // NOLINT
                              metrics->videoDecode ? static_cast<long long>(*metrics->videoDecode) : -1, // NOLINT
                              metrics->audioDecode ? static_cast<long long>(*metrics->audioDecode) : -1, // NOLINT
                              metrics->preRoll ? static_cast<long long>(*metrics->preRoll) : -1,         // NOLINT
                              metrics->play ? static_cast<long long>(*metrics->play) : -1,               // NOLINT
                              metrics->total ? static_cast<long long>(*metrics->total) : -1,             // NOLINT
                              metrics->totalWithoutApp ? static_cast<long long>(*metrics->totalWithoutApp) : -1); // NOLINT
    }
}

GstPadProbeReturn GstProfiler::probeCb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    auto *probeCtx = static_cast<ProbeCtx *>(user_data);
    GstProfiler *self = probeCtx->self;
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    if (!(info->type & GST_PAD_PROBE_TYPE_BUFFER))
        return GST_PAD_PROBE_OK;

    if (!buffer)
        return GST_PAD_PROBE_OK;

    const auto id = self->m_profiler->record(probeCtx->stage, probeCtx->info);
    if (id)
    {
        self->m_profiler->log(id.value());
    }

    return GST_PAD_PROBE_REMOVE;
}

void GstProfiler::probeCtxDestroy(gpointer data)
{
    delete static_cast<ProbeCtx *>(data);
}

std::optional<std::string> GstProfiler::checkElement(GstElement *element)
{
    const gchar *klass = getElementClass(element);
    if (!klass)
        return std::nullopt;

    for (auto token : kKlassTokens)
    {
        if (m_glibWrapper->gStrrstr(klass, token.data()) != nullptr)
        {
            return std::string(token.data()) + " FB Exit";
        }
    }

    return std::nullopt;
}

const gchar *GstProfiler::getElementClass(GstElement *element)
{
    GstElementFactory *factory = m_gstWrapper->gstElementGetFactory(element);
    if (factory)
    {
        return gst_element_factory_get_klass(factory);
    }
    else
    {
        return m_gstWrapper->gstElementClassGetMetadata(GST_ELEMENT_CLASS(G_OBJECT_GET_CLASS(element)),
                                                        GST_ELEMENT_METADATA_KLASS);
    }

    return NULL;
}

std::optional<GstProfiler::PipelineMetrics> GstProfiler::calculateMetrics() const
{
    const auto &records = m_profiler->getRecords();

    PipelineStageTimestamps timestamps;

    for (const auto &record : records)
    {
        const auto &stage = record.stage;
        const auto &info = record.info;

        if (!timestamps.pipelineCreated && stage == "Pipeline Created")
            timestamps.pipelineCreated = record.time;
        else if (!timestamps.allSourcesAttached && stage == "All Sources Attached")
            timestamps.allSourcesAttached = record.time;
        else if (!timestamps.firstSegmentReceivedVideo && stage == "First Segment Received" && info == "Video")
            timestamps.firstSegmentReceivedVideo = record.time;
        else if (!timestamps.firstSegmentReceivedAudio && stage == "First Segment Received" && info == "Audio")
            timestamps.firstSegmentReceivedAudio = record.time;
        else if (!timestamps.sourceFbExitVideo && stage == "Source FB Exit" && info == "Video")
            timestamps.sourceFbExitVideo = record.time;
        else if (!timestamps.sourceFbExitAudio && stage == "Source FB Exit" && info == "Audio")
            timestamps.sourceFbExitAudio = record.time;
        else if (!timestamps.decryptorFbExitVideo && stage == "Decryptor FB Exit" && info == "Video")
            timestamps.decryptorFbExitVideo = record.time;
        else if (!timestamps.decryptorFbExitAudio && stage == "Decryptor FB Exit" && info == "Audio")
            timestamps.decryptorFbExitAudio = record.time;
        else if (!timestamps.decoderFbExitVideo && stage == "Decoder FB Exit" && info == "Video")
            timestamps.decoderFbExitVideo = record.time;
        else if (!timestamps.decoderFbExitAudio && stage == "Decoder FB Exit" && info == "Audio")
            timestamps.decoderFbExitAudio = record.time;
        else if (!timestamps.pipelinePaused && stage == "Pipeline State Changed" && info == "PAUSED")
            timestamps.pipelinePaused = record.time;
        else if (!timestamps.pipelinePlaying && stage == "Pipeline State Changed" && info == "PLAYING")
            timestamps.pipelinePlaying = record.time;
    }

    if (!timestamps.pipelineCreated || !timestamps.allSourcesAttached || !timestamps.firstSegmentReceivedVideo ||
        !timestamps.firstSegmentReceivedAudio || !timestamps.sourceFbExitVideo || !timestamps.sourceFbExitAudio ||
        !timestamps.decoderFbExitVideo || !timestamps.decoderFbExitAudio || !timestamps.pipelinePaused ||
        !timestamps.pipelinePlaying)
    {
        return std::nullopt;
    }

    PipelineMetrics metrics;

    metrics.preparation = diffMs(timestamps.allSourcesAttached, timestamps.pipelineCreated);
    metrics.videoDownload = diffMs(timestamps.firstSegmentReceivedVideo, timestamps.allSourcesAttached);
    metrics.audioDownload = diffMs(timestamps.firstSegmentReceivedAudio, timestamps.allSourcesAttached);
    metrics.videoSource = diffMs(timestamps.sourceFbExitVideo, timestamps.firstSegmentReceivedVideo);
    metrics.audioSource = diffMs(timestamps.sourceFbExitAudio, timestamps.firstSegmentReceivedAudio);

    if (timestamps.decryptorFbExitVideo && timestamps.decryptorFbExitAudio)
    {
        metrics.videoDecryption = diffMs(timestamps.decryptorFbExitVideo, timestamps.sourceFbExitVideo);
        metrics.audioDecryption = diffMs(timestamps.decryptorFbExitAudio, timestamps.sourceFbExitAudio);
        metrics.videoDecode = diffMs(timestamps.decoderFbExitVideo, timestamps.decryptorFbExitVideo);
        metrics.audioDecode = diffMs(timestamps.decoderFbExitAudio, timestamps.decryptorFbExitAudio);
    }
    else
    {
        metrics.videoDecode = diffMs(timestamps.decoderFbExitVideo, timestamps.sourceFbExitVideo);
        metrics.audioDecode = diffMs(timestamps.decoderFbExitAudio, timestamps.sourceFbExitAudio);
    }

    const auto firstMediaReady = maxTime(timestamps.firstSegmentReceivedVideo, timestamps.firstSegmentReceivedAudio);

    metrics.preRoll = diffMs(timestamps.pipelinePaused, firstMediaReady);
    metrics.play = diffMs(timestamps.pipelinePlaying, timestamps.pipelinePaused);
    metrics.total = diffMs(timestamps.pipelinePlaying, timestamps.pipelineCreated);
    metrics.totalWithoutApp = diffMs(timestamps.pipelinePlaying, firstMediaReady);

    return metrics;
}

std::optional<int64_t> GstProfiler::diffMs(const std::optional<Clock::time_point> &end,
                                           const std::optional<Clock::time_point> &start)
{
    if (!end || !start)
        return std::nullopt;

    return std::chrono::duration_cast<std::chrono::milliseconds>(*end - *start).count();
}

std::optional<GstProfiler::Clock::time_point> GstProfiler::maxTime(const std::optional<Clock::time_point> &a,
                                                                   const std::optional<Clock::time_point> &b)
{
    if (a && b)
        return std::max(*a, *b);
    if (a)
        return a;
    if (b)
        return b;
    return std::nullopt;
}

} // namespace firebolt::rialto::server
