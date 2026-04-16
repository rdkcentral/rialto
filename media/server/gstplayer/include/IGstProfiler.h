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

#ifndef FIREBOLT_RIALTO_SERVER_I_GST_PROFILER_H_
#define FIREBOLT_RIALTO_SERVER_I_GST_PROFILER_H_

#include "IGlibWrapper.h"
#include "IGstWrapper.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

struct _GstElement;
using GstElement = _GstElement;

namespace firebolt::rialto::server
{
class IGstProfiler;

class IGstProfilerFactory
{
public:
    using IGstWrapper = firebolt::rialto::wrappers::IGstWrapper;
    using IGlibWrapper = firebolt::rialto::wrappers::IGlibWrapper;

    IGstProfilerFactory() = default;
    virtual ~IGstProfilerFactory() = default;

    static std::shared_ptr<IGstProfilerFactory> getFactory();

    virtual std::unique_ptr<IGstProfiler> createGstProfiler(GstElement *pipeline,
                                                            const std::shared_ptr<IGstWrapper> &gstWrapper,
                                                            const std::shared_ptr<IGlibWrapper> &glibWrapper) const = 0;
};

class IGstProfiler
{
public:
    using RecordId = std::uint64_t;

    virtual ~IGstProfiler() = default;

    virtual bool isEnabled() const = 0;

    virtual std::optional<RecordId> createRecord(std::string stage) = 0;
    virtual std::optional<RecordId> createRecord(std::string stage, std::string info) = 0;

    virtual void scheduleGstElementRecord(GstElement *element) = 0;

    virtual void logRecord(RecordId id) = 0;
    virtual void logPipeline() const = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_GST_PROFILER_H_
