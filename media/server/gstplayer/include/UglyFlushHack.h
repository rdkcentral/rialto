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

#ifndef FIREBOLT_RIALTO_SERVER_UGLY_FLUSH_HACK_H_
#define FIREBOLT_RIALTO_SERVER_UGLY_FLUSH_HACK_H_

#include "MediaCommon.h"
#include <gst/gst.h>
#include <optional>
#include <set>

namespace firebolt::rialto::server
{
/**
 * @brief The workaround class for unresolved gstreamer preroll issue:
 *        https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/150
 */
class UglyFlushHack
{
public:
    UglyFlushHack() = default;
    ~UglyFlushHack() = default;

    bool shouldPostponeFlush(const MediaSourceType &type) const;
    void setFlushing(const MediaSourceType &type, const GstState &currentPipelineState);
    void stateReached(const GstState &newPipelineState);
    void disableHack();

private:
    std::set<MediaSourceType> m_flushingSources{};
    std::optional<GstState> m_targetState{std::nullopt};
    bool m_isPrerolled{false};
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_UGLY_FLUSH_HACK_H_
