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

#ifndef FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
#define FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_

#include "IGlibWrapper.h"
#include "IGstProfiler.h"
#include "IGstWrapper.h"
#include "IProfiler.h"

#include <gst/gst.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace firebolt::rialto::server
{
class GstProfilerFactory : public IGstProfilerFactory
{
public:
    std::unique_ptr<IGstProfiler> createGstProfiler(GstElement *pipeline, const std::shared_ptr<IGstWrapper> &gstWrapper,
                                                    const std::shared_ptr<IGlibWrapper> &glibWrapper) const override;

    static std::weak_ptr<IGstProfilerFactory> m_factory;
};

class GstProfiler : public IGstProfiler
{
public:
    using RecordId = IGstProfiler::RecordId;

    GstProfiler(GstElement *pipeline, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper);
    ~GstProfiler() override;

    bool isEnabled() const override;

    std::optional<RecordId> createRecord(const std::string &stage) override;
    std::optional<RecordId> createRecord(const std::string &stage, const std::string &info) override;

    void scheduleGstElementRecord(GstElement *element) override;
    const std::vector<Record> &getRecords() const override;

    void logRecord(const RecordId id) override;
    void dumpToFile() const override;
    void logPipelineSummary() const override;

private:
    using Clock = std::chrono::system_clock;
    using IGstWrapper = firebolt::rialto::wrappers::IGstWrapper;
    using IGlibWrapper = firebolt::rialto::wrappers::IGlibWrapper;
    using IProfiler = firebolt::rialto::common::IProfiler;

    struct PipelineStageTimestamps
    {
        std::optional<Clock::time_point> pipelineCreated;
        std::optional<Clock::time_point> allSourcesAttached;

        std::optional<Clock::time_point> firstSegmentReceivedVideo;
        std::optional<Clock::time_point> firstSegmentReceivedAudio;

        std::optional<Clock::time_point> sourceFbExitVideo;
        std::optional<Clock::time_point> sourceFbExitAudio;

        std::optional<Clock::time_point> decryptorFbExitVideo;
        std::optional<Clock::time_point> decryptorFbExitAudio;

        std::optional<Clock::time_point> decoderFbExitVideo;
        std::optional<Clock::time_point> decoderFbExitAudio;

        std::optional<Clock::time_point> pipelinePaused;
        std::optional<Clock::time_point> pipelinePlaying;
    };

    struct PipelineMetrics
    {
        std::optional<int64_t> preparation;
        std::optional<int64_t> videoDownload;
        std::optional<int64_t> audioDownload;
        std::optional<int64_t> videoSource;
        std::optional<int64_t> audioSource;
        std::optional<int64_t> videoDecryption;
        std::optional<int64_t> audioDecryption;
        std::optional<int64_t> videoDecode;
        std::optional<int64_t> audioDecode;
        std::optional<int64_t> preRoll;
        std::optional<int64_t> play;
        std::optional<int64_t> total;
        std::optional<int64_t> totalWithoutApp;
    };

    std::optional<std::string> getFirstBufferExitStage(GstElement *element);
    const gchar *getElementClassMetadata(GstElement *element);
    std::string deriveElementInfoFromName(const std::string &name) const;

    std::optional<GstProfiler::PipelineMetrics> calculateMetrics() const;

    GstElement *m_pipeline = nullptr;
    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::shared_ptr<IGlibWrapper> m_glibWrapper;
    std::shared_ptr<IProfiler> m_profiler;
    bool m_enabled = false;
};
} // namespace firebolt::rialto::server
#endif // FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
