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

#ifndef FIREBOLT_RIALTO_SERVER_WEB_AUDIO_PLAYER_CONTEXT_H_
#define FIREBOLT_RIALTO_SERVER_WEB_AUDIO_PLAYER_CONTEXT_H_

#include "IGstSrc.h"
#include "MediaCommon.h"
#include <gst/gst.h>
#include <list>
#include <map>
#include <memory>

namespace firebolt::rialto::server
{
struct WebAudioPlayerContext
{
    /**
     * @brief The rialto src object.
     */
    std::shared_ptr<IGstSrc> gstSrc{nullptr};

    /**
     * @brief The audio app source
     */
    GstElement *audioAppSrc{nullptr};

    /**
     * @brief The gstreamer pipeline.
     */
    GstElement *pipeline{nullptr};

    /**
     * @brief The gstreamer source.
     */
    GstElement *source{nullptr};

    /**
     * @brief List containing audio buffers to attach
     *
     * List can be used only in worker thread
     */
    std::list<GstBuffer *> audioBuffers{};
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_WEB_AUDIO_PLAYER_CONTEXT_H_
