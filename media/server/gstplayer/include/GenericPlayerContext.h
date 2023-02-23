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

#ifndef FIREBOLT_RIALTO_SERVER_GENERIC_PLAYER_CONTEXT_H_
#define FIREBOLT_RIALTO_SERVER_GENERIC_PLAYER_CONTEXT_H_

#include "IGstSrc.h"
#include "IRdkGstreamerUtilsWrapper.h"
#include "ITimer.h"
#include "MediaCommon.h"
#include <gst/gst.h>
#include <list>
#include <map>
#include <memory>

namespace firebolt::rialto::server
{
constexpr double kNoPendingPlaybackRate{0.0};

/**
 * @brief Definition of a stream info map.
 */
using StreamInfoMap = std::map<MediaSourceType, GstElement *>;

/**
 * @brief Structure used for video geometry
 */
struct Rectangle
{
    int x, y, width, height;
    constexpr Rectangle() : x(0), y(0), width(0), height(0) {}
    constexpr Rectangle(int x_, int y_, int w_, int h_) : x(x_), y(y_), width(w_), height(h_) {}
    Rectangle(const Rectangle &rhs) = default;
    inline constexpr bool empty() { return (width == 0) || (height == 0); }
    inline void clear() { x = y = width = height = 0; }
};

struct GenericPlayerContext
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
     * @brief The video app source
     */
    GstElement *videoAppSrc{nullptr};

    /**
     * @brief The gstreamer pipeline.
     */
    GstElement *pipeline{nullptr};

    /**
     * @brief The gstreamer source.
     */
    GstElement *source{nullptr};

    /**
     * @brief A map of streams attached to the source.
     */
    StreamInfoMap streamInfo{};
    /**
     * @brief Flag used to check, if we need to request for new audio data.
     *
     * Flag can be used only in worker thread
     */
    bool audioNeedData{false};

    /**
     * @brief Flag used to check, if we need to request for new video data.
     *
     * Flag can be used only in worker thread
     */
    bool videoNeedData{false};

    /**
     * @brief Flag used to check, if request for audio data was sent and we didn't receive response yet.
     *
     * Flag can be used only in worker thread
     */
    bool audioNeedDataPending{false};

    /**
     * @brief Flag used to check, if request for video data was sent and we didn't receive response yet.
     *
     * Flag can be used only in worker thread
     */
    bool videoNeedDataPending{false};

    /**
     * @brief Flag used to check, if any audio data has been pushed to gstreamer (to check if BUFFERED can be sent)
     *
     * Flag can be used only in worker thread
     */
    bool audioDataPushed{false};

    /**
     * @brief Flag used to check, if any video data has been pushed to gstreamer (to check if BUFFERED can be sent)
     *
     * Flag can be used only in worker thread
     */
    bool videoDataPushed{false};

    /**
     * @brief Flag used to check, if BUFFERED notification has been sent.
     *
     * Flag can be used only in worker thread
     */
    bool bufferedNotificationSent{false};

    /**
     * @brief List containing audio buffers to attach
     *
     * List can be used only in worker thread
     */
    std::list<GstBuffer *> audioBuffers{};

    /**
     * @brief List containing video buffers to attach
     *
     * List can be used only in worker thread
     */
    std::list<GstBuffer *> videoBuffers{};

    /**
     * @brief Flag used to check, if audio underflow callback occured
     *
     * Flag can be used only in worker thread
     */
    bool audioUnderflowOccured{false};

    /**
     * @brief Flag used to check, if video underflow callback occured
     *
     * Flag can be used only in worker thread
     */
    bool videoUnderflowOccured{false};

    /**
     * @brief Flag used to check, if audio underflow reporting is enabled
     *
     * Flag can be used only in worker thread
     */
    bool audioUnderflowEnabled{false};

    /**
     * @brief Pending video geometry
     */
    Rectangle pendingGeometry;

    /**
     * @brief Current playback rate
     */
    double playbackRate{1.0};

    /**
     * @brief Pending playback rate
     */
    double pendingPlaybackRate{kNoPendingPlaybackRate};

    /**
     * @brief Last audio sample timestamps
     * TODO(LLDEV-31012) Needed to detect audio stream underflow
     */
    int64_t lastAudioSampleTimestamps{0};

    /**
     * @brief The decryption service.
     */
    IDecryptionService *decryptionService{nullptr};

    /**
     * @brief Flag used to check, if audio source has been recently removed
     *
     * Flag can be used only in worker thread
     */
    bool audioSourceRemoved{false};

    /**
     * @brief Audio elements of gst pipeline.
     *
     * Attribute can be used only in worker thread
     */
    PlaybackGroupPrivate playbackGroup;

    /**
     * @brief Signals connected to pipeline elements
     *
     * Attribute can be used only in worker thread
     */
    std::map<GstElement *, gulong> connectedSignals;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GENERIC_PLAYER_CONTEXT_H_
