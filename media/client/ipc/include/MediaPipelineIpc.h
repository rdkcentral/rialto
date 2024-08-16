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

#ifndef FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_IPC_H_

#include "IEventThread.h"
#include "IMediaPipelineIpc.h"
#include "IpcModule.h"
#include <IMediaPipeline.h>
#include <memory>
#include <string>

#include "mediapipelinemodule.pb.h"

namespace firebolt::rialto::client
{
/**
 * @brief IMediaPipelineIpc factory class definition.
 */
class MediaPipelineIpcFactory : public IMediaPipelineIpcFactory
{
public:
    /**
     * @brief Weak pointer to the singleton factory object.
     */
    static std::weak_ptr<IMediaPipelineIpcFactory> m_factory;

    std::unique_ptr<IMediaPipelineIpc> createMediaPipelineIpc(IMediaPipelineIpcClient *client,
                                                              const VideoRequirements &videoRequirements,
                                                              std::weak_ptr<IIpcClient> ipcClient) override;
};

/**
 * @brief The definition of the MediaPipelineIpc.
 */
class MediaPipelineIpc : public IMediaPipelineIpc, public IpcModule
{
public:
    /**
     * @brief The constructor.
     *
     * @param[in] client                : The Rialto ipc media player client.
     * @param[in] videoRequirements     : The video decoder requirements for the MediaPipeline session.
     * @param[in] ipcClient             : The ipc client
     * @param[in] eventThreadFactory    : The event thread factory
     */
    MediaPipelineIpc(IMediaPipelineIpcClient *client, const VideoRequirements &videoRequirements, IIpcClient &ipcClient,
                     const std::shared_ptr<common::IEventThreadFactory> &eventThreadFactory);

    /**
     * @brief Virtual destructor.
     */
    virtual ~MediaPipelineIpc();

    bool attachSource(const std::unique_ptr<IMediaPipeline::MediaSource> &source, int32_t &sourceId) override;

    bool removeSource(int32_t sourceId) override;

    bool allSourcesAttached() override;

    bool load(MediaType type, const std::string &mimeType, const std::string &url) override;

    bool setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

    bool play() override;

    bool pause() override;

    bool stop() override;

    bool haveData(MediaSourceStatus status, uint32_t numFrames, uint32_t requestId) override;

    bool setPosition(int64_t position) override;

    bool getPosition(int64_t &position) override;

    bool setPlaybackRate(double rate) override;

    bool renderFrame() override;

    bool setVolume(double volume) override;

    bool getVolume(double &volume) override;

    bool setMute(int32_t sourceId, bool mute) override;

    bool getMute(int32_t sourceId, bool &mute) override;

    bool flush(int32_t sourceId, bool resetTime) override;

    bool setSourcePosition(int32_t sourceId, int64_t position) override;

private:
    /**
     * @brief The media player client ipc.
     */
    IMediaPipelineIpcClient *m_mediaPipelineIpcClient;

    /**
     * @brief The ipc protobuf media player stub.
     */
    std::unique_ptr<::firebolt::rialto::MediaPipelineModule_Stub> m_mediaPipelineStub;

    /**
     * @brief The session id of the current session.
     */
    // onPlaybackStateUpdated() can be called before createSession() therefore
    // initialise m_sessionId to -1 which is an invalid session_id
    std::atomic<int> m_sessionId{-1};

    /**
     * @brief Thread for handling media player events from the server.
     */
    std::unique_ptr<common::IEventThread> m_eventThread;

    bool createRpcStubs(const std::shared_ptr<ipc::IChannel> &ipcChannel) override;

    bool subscribeToEvents(const std::shared_ptr<ipc::IChannel> &ipcChannel) override;

    /**
     * @brief Handler for a playback state update from the server.
     *
     * @param[in] event : The player state changed event structure.
     */
    void onPlaybackStateUpdated(const std::shared_ptr<firebolt::rialto::PlaybackStateChangeEvent> &event);

    /**
     * @brief Handler for a playback position update from the server.
     *
     * @param[in] event : The playback position changed event structure.
     */
    void onPositionUpdated(const std::shared_ptr<firebolt::rialto::PositionChangeEvent> &event);

    /**
     * @brief Handler for a network state update from the server.
     *
     * @param[in] event : The player state changed event structure.
     */
    void onNetworkStateUpdated(const std::shared_ptr<firebolt::rialto::NetworkStateChangeEvent> &event);

    /**
     * @brief Handler for a need data request from the server.
     *
     * @param[in] event : The need data event structure.
     */
    void onNeedMediaData(const std::shared_ptr<firebolt::rialto::NeedMediaDataEvent> &event);

    /**
     * @brief Handler for a QOS update from the server.
     *
     * @param[in] event : The qos event structure.
     */
    void onQos(const std::shared_ptr<firebolt::rialto::QosEvent> &event);

    /**
     * @brief Handler for a buffer underflow notification from the server.
     *
     * @param[in] event : The buffer underflow event structure.
     */
    void onBufferUnderflow(const std::shared_ptr<firebolt::rialto::BufferUnderflowEvent> &event);

    /**
     * @brief Handler for a playback error notification from the server.
     *
     * @param[in] event : The playback error event structure.
     */
    void onPlaybackError(const std::shared_ptr<firebolt::rialto::PlaybackErrorEvent> &event);

    /**
     * @brief Handler for a source flushed notification from the server.
     *
     * @param[in] event : The source flushed event structure.
     */
    void onSourceFlushed(const std::shared_ptr<firebolt::rialto::SourceFlushedEvent> &event);

    /**
     * @brief Create a new player session.
     *
     * @param[in] videoRequirements : The video decoder requirements for the MediaPipeline session.
     *
     * @retval true on success, false otherwise.
     */
    bool createSession(const VideoRequirements &videoRequirements);

    /**
     * @brief Destroy the current player session.
     */
    void destroySession();

    /**
     * @brief Converts the MediaType enum to protobuf LoadRequest MediaType.
     */
    firebolt::rialto::LoadRequest_MediaType convertLoadRequestMediaType(MediaType mediaType);

    /**
     * @brief Converts the MediaSourceStatus enum to protobuf HaveDataRequest MediaSourceStatus.
     */
    firebolt::rialto::HaveDataRequest_MediaSourceStatus convertHaveDataRequestMediaSourceStatus(MediaSourceStatus status);

    /**
     * @brief Converts the SegmentAlignment enum to protobuf AttachSourceRequest SegmentAlignment.
     */
    firebolt::rialto::AttachSourceRequest_SegmentAlignment
    convertSegmentAlignment(const firebolt::rialto::SegmentAlignment &alignment);

    /**
     * @brief Converts the StreamFormat enum to protobuf AttachSourceRequest StreamFormat.
     */
    firebolt::rialto::AttachSourceRequest_StreamFormat
    convertStreamFormat(const firebolt::rialto::StreamFormat &streamFormat);

    firebolt::rialto::AttachSourceRequest_ConfigType
    convertConfigType(const firebolt::rialto::SourceConfigType &configType);

    /**
     * @brief Converts the CodecDataType enum to protobuf AttachSourceRequest CodecDataType.
     */
    firebolt::rialto::AttachSourceRequest_CodecData_Type
    convertCodecDataType(const firebolt::rialto::CodecDataType &codecDataType);
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_IPC_H_
