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

#include <memory>
#include <optional>
#include <string>

namespace firebolt::rialto::server
{
class GstProfiler : public IGstProfiler
{
public:
    using IGstWrapper = firebolt::rialto::wrappers::IGstWrapper;
    using IGlibWrapper = firebolt::rialto::wrappers::IGlibWrapper;
    using IProfiler = firebolt::rialto::common::IProfiler;
    using RecordId = IGstProfiler::RecordId;

    GstProfiler(GstElement *pipeline, const std::shared_ptr<IGstWrapper> &gstWrapper,
                const std::shared_ptr<IGlibWrapper> &glibWrapper);
    ~GstProfiler() override;

    std::optional<RecordId> createRecord(std::string stage) override;
    std::optional<RecordId> createRecord(std::string stage, std::string info) override;

    void scheduleGstElementRecord(GstElement *element) override;

    void logRecord(const RecordId id) override;

private:
    struct ProbeCtx
    {
        GstProfiler *self;
        std::string stage;
        std::string info;
    };

    std::optional<std::string> checkElement(GstElement *element);

    static GstPadProbeReturn probeCb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static void probeCtxDestroy(gpointer data);

    GstElement *m_pipeline = nullptr;
    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::shared_ptr<IGlibWrapper> m_glibWrapper;
    std::unique_ptr<IProfiler> m_profiler;
    bool m_enabled = false;
    static constexpr std::string_view k_module = "GstProfiler";
};
} // namespace firebolt::rialto::server
#endif // FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
