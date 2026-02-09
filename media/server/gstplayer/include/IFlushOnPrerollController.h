/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_I_FLUSH_ONPREROLL_CONTROLLER_H_
#define FIREBOLT_RIALTO_SERVER_I_FLUSH_ONPREROLL_CONTROLLER_H_

#include "MediaCommon.h"
#include <gst/gst.h>

namespace firebolt::rialto::server
{
/**
 * @brief The workaround class for unresolved gstreamer preroll issue:
 *        https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/150
 */
class IFlushOnPrerollController
{
public:
    virtual ~IFlushOnPrerollController() = default;

    virtual void waitIfRequired(const MediaSourceType &type) = 0;
    virtual void setFlushing(const MediaSourceType &type) = 0;
    virtual void setPrerolling() = 0;
    virtual void stateReached(const GstState &newPipelineState) = 0;
    virtual void setTargetState(const GstState &state) = 0;
    virtual void reset() = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_FLUSH_ONPREROLL_CONTROLLER_H_
