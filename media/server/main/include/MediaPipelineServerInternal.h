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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_PIPELINE_SERVER_INTERNAL_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_PIPELINE_SERVER_INTERNAL_H_

#include "DataReaderFactory.h"
#include "IActiveRequests.h"
#include "IGstGenericPlayer.h"
#include "IMainThread.h"
#include "IMediaPipelineServerInternal.h"
#include "ITimer.h"
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace firebolt::rialto::server
{
/**
 * @brief IMediaPipelineServerInternal factory class definition.
 */
class MediaPipelineServerInternalFactory : public server::IMediaPipelineServerInternalFactory
{
public:
    MediaPipelineServerInternalFactory() = default;
    ~MediaPipelineServerInternalFactory() override = default;

    std::unique_ptr<IMediaPipeline> createMediaPipeline(std::weak_ptr<IMediaPipelineClient> client,
                                                        const VideoRequirements &videoRequirements) const override;

    std::unique_ptr<server::IMediaPipelineServerInternal> createMediaPipelineServerInternal(
        std::weak_ptr<IMediaPipelineClient> client, const VideoRequirements &videoRequirements, int sessionId,
        const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer, IDecryptionService &decryptionService) const override;

    /**
     * @brief Create the generic media player factory object.
     *
     * @retval the generic media player factory instance or null on error.
     */
    static std::shared_ptr<MediaPipelineServerInternalFactory> createFactory();
};

/**
 * @brief The definition of the MediaPipelineServerInternal.
 */
class MediaPipelineServerInternal : public IMediaPipelineServerInternal, public IGstGenericPlayerClient
{
public:
    /**
     * @brief The constructor.
     *
     * @param[in] client            : The Rialto media player client.
     * @param[in] videoRequirements : The video decoder requirements for the MediaPipeline session.
     * @param[in] gstPlayerFactory  : The gstreamer player factory.
     * @param[in] sessionId         : The session id
     * @param[in] shmBuffer         : The shared memory buffer
     * @param[in] mainThreadFactory : The main thread factory.
     * @param[in] dataReaderFactory : The data reader factory
     * @param[in] activeRequests    : The active requests
     * @param[in] decryptionService : The decryption service
     */
    MediaPipelineServerInternal(std::shared_ptr<IMediaPipelineClient> client, const VideoRequirements &videoRequirements,
                                const std::shared_ptr<IGstGenericPlayerFactory> &gstPlayerFactory, int sessionId,
                                const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer,
                                const std::shared_ptr<IMainThreadFactory> &mainThreadFactory,
                                std::shared_ptr<common::ITimerFactory> timerFactory,
                                std::unique_ptr<IDataReaderFactory> &&dataReaderFactory,
                                std::unique_ptr<IActiveRequests> &&activeRequests, IDecryptionService &decryptionService);

    /**
     * @brief Virtual destructor.
     */
    virtual ~MediaPipelineServerInternal();

    bool load(MediaType type, const std::string &mimeType, const std::string &url) override;

    bool attachSource(const std::unique_ptr<MediaSource> &source) override;

    bool removeSource(int32_t id) override;

    bool allSourcesAttached() override;

    bool play() override;

    bool pause() override;

    bool stop() override;

    bool setPlaybackRate(double rate) override;

    bool setPosition(int64_t position) override;

    bool getPosition(int64_t &position) override;

    bool setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

    bool haveData(MediaSourceStatus status, uint32_t needDataRequestId) override;

    bool haveData(MediaSourceStatus status, uint32_t numFrames, uint32_t needDataRequestId) override;

    void ping(std::unique_ptr<IHeartbeatHandler> &&heartbeatHandler) override;

    bool renderFrame() override;

    bool setVolume(double volume) override;

    bool getVolume(double &volume) override;

    bool setMute(bool mute) override;

    bool getMute(bool &mute) override;

    bool flush(int32_t sourceId, bool resetTime) override;

    AddSegmentStatus addSegment(uint32_t needDataRequestId, const std::unique_ptr<MediaSegment> &mediaSegment) override;

    std::weak_ptr<IMediaPipelineClient> getClient() override;

    void notifyPlaybackState(PlaybackState state) override;

    bool notifyNeedMediaData(MediaSourceType mediaSourceType) override;

    void notifyPosition(std::int64_t position) override;

    void notifyNetworkState(NetworkState state) override;

    void clearActiveRequestsCache() override;

    void invalidateActiveRequests(const MediaSourceType &type) override;

    void notifyQos(MediaSourceType mediaSourceType, const QosInfo &qosInfo) override;

    void notifyBufferUnderflow(MediaSourceType mediaSourceType) override;

    void notifyPlaybackError(MediaSourceType mediaSourceType, PlaybackError error) override;

    void notifySourceFlushed(MediaSourceType mediaSourceType) override;

protected:
    /**
     * @brief The media player client.
     */
    std::shared_ptr<IMediaPipelineClient> m_mediaPipelineClient;

    /**
     * @brief The mainThread object.
     */
    std::shared_ptr<IMainThread> m_mainThread;

    /**
     * @brief The gstreamer player factory object.
     */
    const std::shared_ptr<IGstGenericPlayerFactory> m_kGstPlayerFactory;

    /**
     * @brief The gstreamer player.
     */
    std::unique_ptr<IGstGenericPlayer> m_gstPlayer;

    /**
     * @brief The video decoder requirements for the MediaPipeline session.
     */
    const VideoRequirements m_kVideoRequirements;

    /**
     * @brief ID of a session represented by this MediaPipeline
     */
    int m_sessionId;

    /**
     * @brief Shared memory buffer
     */
    std::shared_ptr<ISharedMemoryBuffer> m_shmBuffer;

    /**
     * @brief DataReader factory
     */
    std::unique_ptr<IDataReaderFactory> m_dataReaderFactory;

    /**
     * @brief Factory creating timers
     */
    std::shared_ptr<common::ITimerFactory> m_timerFactory;

    /**
     * @brief Object containing all active NeedDataRequests
     */
    std::unique_ptr<IActiveRequests> m_activeRequests;

    /**
     * @brief This objects id registered on the main thread
     */
    uint32_t m_mainThreadClientId;

    /**
     * @brief Decryption service
     */
    IDecryptionService &m_decryptionService;

    /**
     * @brief Current playback state
     */
    PlaybackState m_currentPlaybackState;

    /**
     * @brief Map containing scheduled need media data requests.
     */
    std::unordered_map<MediaSourceType, std::unique_ptr<firebolt::rialto::common::ITimer>> m_needMediaDataTimers;

    /**
     * @brief Currently attached sources
     */
    std::map<MediaSourceType, std::int32_t> m_attachedSources;

    /**
     * @brief Map to keep track of the count of MediaSourceStatus with the value NO_AVAILABLE_SAMPLES for each MediaSource
     */
    std::map<MediaSourceType, unsigned int> m_noAvailableSamplesCounter;

    /**
     * @brief Flag used to check if allSourcesAttached was already called
     */
    bool m_wasAllSourcesAttachedCalled;

    /**
     * @brief Map of flags used to check if Eos has been set on the media type for this playback
     */
    std::map<MediaSourceType, bool> m_isMediaTypeEosMap;

    /**
     * @brief Load internally, only to be called on the main thread.
     *
     * @param[in] type     : The media type.
     * @param[in] mimeType : The MIME type.
     * @param[in] url      : The URL.
     *
     * @retval true on success.
     */
    bool loadInternal(MediaType type, const std::string &mimeType, const std::string &url);

    /**
     * @brief Attach source internally, only to be called on the main thread.
     *
     * @param[in] source : The source.
     *
     * @retval true on success.
     */
    bool attachSourceInternal(const std::unique_ptr<MediaSource> &source);

    /**
     * @brief Remove source internally, only to be called on the main thread.
     *
     * @param[in] id : The source id.
     *
     * @retval true on success.
     */
    bool removeSourceInternal(int32_t id);

    /**
     * @brief Notify all sources attached internally, only to be called on the main thread.
     *
     * @retval true on success.
     */
    bool allSourcesAttachedInternal();

    /**
     * @brief Play internally, only to be called on the main thread.
     *
     * @retval true on success.
     */
    bool playInternal();

    /**
     * @brief Pause internally, only to be called on the main thread.
     *
     * @retval true on success.
     */
    bool pauseInternal();

    /**
     * @brief Stop internally, only to be called on the main thread.
     *
     * @retval true on success.
     */
    bool stopInternal();

    /**
     * @brief Set the playback rate internally, only to be called on the main thread.
     *
     * @param[in] rate : The playback rate.
     *
     * @retval true on success.
     */
    bool setPlaybackRateInternal(double rate);

    /**
     * @brief Set the position internally, only to be called on the main thread.
     *
     * @param[in] position : The playback position in nanoseconds.
     *
     * @retval true on success.
     */
    bool setPositionInternal(int64_t position);

    /**
     * @brief Get position internally, only to be called on the main thread.
     *
     * @param[out] position : The playback position in nanoseconds
     *
     * @retval true on success.
     */
    bool getPositionInternal(int64_t &position);

    /**
     * @brief Set video window internally, only to be called on the main thread.
     *
     * @param[in] x      : The x position in pixels.
     * @param[in] y      : The y position in pixels.
     * @param[in] width  : The width in pixels.
     * @param[in] height : The height in pixels.
     *
     * @retval true on success.
     */
    bool setVideoWindowInternal(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    /**
     * @brief Have data internally, only to be called on the main thread.
     *
     * @param[in] status : The status
     * @param[in] needDataRequestId : Need data request id
     *
     * @retval true on success.
     */
    bool haveDataInternal(MediaSourceStatus status, uint32_t needDataRequestId);

    /**
     * @brief Render frame internally, only to be called on the main thread.
     *
     * @retval true on success.
     */
    bool renderFrameInternal();

    /**
     * @brief Have data internally, only to be called on the main thread.
     *
     * @param[in] status            : The status
     * @param[in] numFrames         : The number of frames written.
     * @param[in] needDataRequestId : Need data request id
     *
     * @retval true on success.
     */
    bool haveDataInternal(MediaSourceStatus status, uint32_t numFrames, uint32_t needDataRequestId);

    /**
     * @brief Add segment internally, only to be called on the main thread.
     *
     * @param[in] needDataRequestId : The status
     * @param[in] mediaSegment : The data returned.
     *
     * @retval status of adding segment
     */
    AddSegmentStatus addSegmentInternal(uint32_t needDataRequestId, const std::unique_ptr<MediaSegment> &mediaSegment);

    /**
     * @brief Notify need media data internally, only to be called on the main thread.
     *
     * @param[in] mediaSourceType    : The media source type.
     */
    bool notifyNeedMediaDataInternal(MediaSourceType mediaSourceType);

    /**
     * @brief Schedules resending of NeedMediaData after a short delay. Used when no segments were received in the
     * haveData() call to prevent a storm of needData()/haveData() calls, only to be called on the main thread.
     *
     * @param[in] mediaSourceType    : The media source type.
     */
    void scheduleNotifyNeedMediaData(MediaSourceType mediaSourceType);

    /**
     * @brief Set volume internally, only to be called on the main thread.
     *
     * @param[in] volume Target volume level (0.0 - 1.0)
     *
     * @retval true on success false otherwise
     */
    bool setVolumeInternal(double volume);

    /**
     * @brief Get volume internally, only to be called on the main thread.
     *
     * @param[out] volume Current volume level (range 0.0 - 1.0)
     *
     * @retval true on success false otherwise
     */
    bool getVolumeInternal(double &volume);

    /**
     * @brief Set mute internally, only to be called on the main thread.
     *
     * @param[in] mute Desired mute state, true=muted, false=not muted
     *
     * @retval true on success false otherwise
     */
    bool setMuteInternal(bool mute);

    /**
     * @brief Get mute internally, only to be called on the main thread.
     *
     * @param[out] mute Current mute state
     *
     * @retval true on success false otherwise
     */
    bool getMuteInternal(bool &mute);

    /**
     * @brief Checks if MediaPipeline threads are not deadlocked internally
     *
     * @param[out] heartbeatHandler : The heartbeat handler instance
     */
    void pingInternal(std::unique_ptr<IHeartbeatHandler> &&heartbeatHandler);

    /**
     * @brief Flushes a source.
     *
     * @param[in] sourceId  : The source id. Value should be set to the MediaSource.id returned after attachSource()
     * @param[in] resetTime : True if time should be reset
     *
     * @retval true on success.
     */
    virtual bool flushInternal(int32_t sourceId, bool resetTime);
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_PIPELINE_SERVER_INTERNAL_H_
