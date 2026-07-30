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

#ifndef FIREBOLT_RIALTO_SERVER_I_GST_PROFILER_PRIVATE_H_
#define FIREBOLT_RIALTO_SERVER_I_GST_PROFILER_PRIVATE_H_

#include <gst/gst.h>

namespace firebolt::rialto::server
{
class IGstProfilerPrivate
{
public:
    IGstProfilerPrivate() = default;
    virtual ~IGstProfilerPrivate() = default;

    IGstProfilerPrivate(const IGstProfilerPrivate &) = delete;
    IGstProfilerPrivate &operator=(const IGstProfilerPrivate &) = delete;
    IGstProfilerPrivate(IGstProfilerPrivate &&) = delete;
    IGstProfilerPrivate &operator=(IGstProfilerPrivate &&) = delete;

    /**
     * @brief Handles pad probe callback.
     *
     * @param[in] pad : Pad where the probe is attached.
     * @param[in] info : Probe info.
     *
     * @retval The probe return code.
     */
    virtual GstPadProbeReturn handleProbeCb(GstPad *pad, GstPadProbeInfo *info) = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_GST_PROFILER_PRIVATE_H_
