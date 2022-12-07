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

#ifndef FIREBOLT_RIALTO_SERVER_I_GST_PLAYER_PRIVATE_H_
#define FIREBOLT_RIALTO_SERVER_I_GST_PLAYER_PRIVATE_H_

#include "IMediaPipeline.h"
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

namespace firebolt::rialto::server
{
class IGstPlayerPrivate
{
public:
    IGstPlayerPrivate() = default;
    virtual ~IGstPlayerPrivate() = default;

    IGstPlayerPrivate(const IGstPlayerPrivate &) = delete;
    IGstPlayerPrivate &operator=(const IGstPlayerPrivate &) = delete;
    IGstPlayerPrivate(IGstPlayerPrivate &&) = delete;
    IGstPlayerPrivate &operator=(IGstPlayerPrivate &&) = delete;

    /**
     * @brief Schedules source setup finish task. Called by the worker thread.
     */
    virtual void scheduleSourceSetupFinish() = 0;

    /**
     * @brief Schedules need media data task. Called by the worker thread.
     */
    virtual void scheduleNeedMediaData(GstAppSrc *src) = 0;

    /**
     * @brief Schedules enough data task. Called by the worker thread.
     */
    virtual void scheduleEnoughData(GstAppSrc *src) = 0;

    /**
     * @brief Schedules audio underflow task. Called by the worker thread.
     */
    virtual void scheduleAudioUnderflow() = 0;

    /**
     * @brief Schedules video underflow task. Called by the worker thread.
     */
    virtual void scheduleVideoUnderflow() = 0;

    /**
     * @brief Sets Westeros sink rectangle. Called by the worker thread.
     *
     * @retval true on success.
     */
    virtual bool setWesterossinkRectangle() = 0;

    /**
     * @brief Sets Westeros sink res-usage flag for a secondary video.
     *
     * @retval true on success.
     */
    virtual bool setWesterossinkSecondaryVideo() = 0;

    /**
     * @brief Sends NeedMediaData notification. Called by the worker thread.
     */
    virtual void notifyNeedMediaData(bool audioNotificationNeeded, bool videoNotificationNeeded) = 0;

    /**
     * @brief Constructs a new buffer with data from media segment. Performs decryption, if needed.
     *        Called by the worker thread.
     */
    virtual GstBuffer *createDecryptedBuffer(const IMediaPipeline::MediaSegment &mediaSegment) const = 0;

    /**
     * @brief Constructs a new buffer with data from media segment. Performs decryption, if needed.
     *        Called by the worker thread.
     */
    virtual GstBuffer *createBuffer(const IMediaPipeline::MediaSegment &mediaSegment) const = 0;

    /**
     * @brief Attach audio data. Called by the worker thread
     */
    virtual void attachAudioData() = 0;

    /**
     * @brief Attach video data. Called by the worker thread
     */
    virtual void attachVideoData() = 0;

    /**
     * @brief Checks the new audio mediaSegment metadata and updates the caps accordingly.
     */
    virtual void updateAudioCaps(int32_t rate, int32_t channels) = 0;

    /**
     * @brief Checks the new video mediaSegment metadata and updates the caps accordingly.
     */
    virtual void updateVideoCaps(int32_t width, int32_t height) = 0;

    /**
     * @brief Changes pipeline state.
     *
     * @param[in] newState    : The desired state.
     *
     * @retval true on success.
     */
    virtual bool changePipelineState(GstState newState) = 0;

    /**
     * @brief Starts position reporting and check audio underflow. Called by the worker thread.
     */
    virtual void startPositionReportingAndCheckAudioUnderflowTimer() = 0;

    /**
     * @brief Stops position reporting and check audio underflow. Called by the worker thread.
     */
    virtual void stopPositionReportingAndCheckAudioUnderflowTimer() = 0;

    /**
     * @brief Stops worker thread. Called by the worker thread.
     */
    virtual void stopWorkerThread() = 0;

    /**
     * @brief Restores playback after underflow. Called by the worker thread.
     *
     * @param[in] underflowFlag    : The audio or video underflow flag to be cleared.
     */
    virtual void cancelUnderflow(bool &underflowFlag) = 0;

    /**
     * @brief Sets pending playback rate after reaching PLAYING state
     *
     */
    virtual void setPendingPlaybackRate() = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_GST_PLAYER_PRIVATE_H_
