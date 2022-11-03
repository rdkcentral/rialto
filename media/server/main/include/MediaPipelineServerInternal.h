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
#include "IGstPlayer.h"
#include "IMediaPipelineServerInternal.h"
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server
{
class MediaPipelineServerInternal;

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
class MediaPipelineServerInternal : public IMediaPipelineServerInternal, public IGstPlayerClient
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
     * @param[in] dataReaderFactory : The data reader factory
     * @param[in] activeRequests    : The active requests
     * @param[in] decryptionService : The decryption service
     */
    MediaPipelineServerInternal(std::shared_ptr<IMediaPipelineClient> client, const VideoRequirements &videoRequirements,
                                const std::shared_ptr<IGstPlayerFactory> &gstPlayerFactory, int sessionId,
                                const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer,
                                std::unique_ptr<IDataReaderFactory> &&dataReaderFactory,
                                std::unique_ptr<IActiveRequests> &&activeRequests, IDecryptionService &decryptionService);

    /**
     * @brief Virtual destructor.
     */
    virtual ~MediaPipelineServerInternal();

    bool load(MediaType type, const std::string &mimeType, const std::string &url) override;

    bool attachSource(MediaSource &source) override;

    bool removeSource(int32_t id) override;

    bool play() override;

    bool pause() override;

    bool stop() override;

    bool setPlaybackRate(double rate) override;

    bool setPosition(int64_t position) override;

    bool getPosition(int64_t &position) override;

    bool setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

    bool haveData(MediaSourceStatus status, uint32_t needDataRequestId) override;

    bool haveData(MediaSourceStatus status, uint32_t numFrames, uint32_t needDataRequestId) override;

    AddSegmentStatus addSegment(uint32_t needDataRequestId, const std::unique_ptr<MediaSegment> &mediaSegment) override;

    std::weak_ptr<IMediaPipelineClient> getClient() override;

    void notifyPlaybackState(PlaybackState state) override;

    bool notifyNeedMediaData(MediaSourceType mediaSourceType) override;

    void notifyPosition(std::int64_t position) override;

    void notifyNetworkState(NetworkState state) override;

    void clearActiveRequestsCache() override;

    void notifyQos(MediaSourceType mediaSourceType, const QosInfo &qosInfo) override;

protected:
    /**
     * @brief The media player client.
     */
    std::shared_ptr<IMediaPipelineClient> m_mediaPipelineClient;

    /**
     * @brief The gstreamer player factory object.
     */
    const std::shared_ptr<IGstPlayerFactory> m_kGstPlayerFactory;

    /**
     * @brief The gstreamer player.
     */
    std::unique_ptr<IGstPlayer> m_gstPlayer;

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
     * @brief Object containing all active NeedDataRequests
     */
    std::unique_ptr<IActiveRequests> m_activeRequests;

    /**
     * @brief Decryption service
     */
    IDecryptionService &m_decryptionService;
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_PIPELINE_SERVER_INTERNAL_H_
