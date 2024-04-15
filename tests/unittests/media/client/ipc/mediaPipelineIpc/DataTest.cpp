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

#include "MediaPipelineIpcTestBase.h"
#include "MediaPipelineProtoRequestMatchers.h"

MATCHER(IsNull, "")
{
    return arg == nullptr;
}

MATCHER_P(ShmInfoMatcher, shmInfo, "")
{
    return ((arg->maxMetadataBytes == shmInfo->maxMetadataBytes) && (arg->metadataOffset == shmInfo->metadataOffset) &&
            (arg->mediaDataOffset == shmInfo->mediaDataOffset) && (arg->maxMediaBytes == shmInfo->maxMediaBytes));
}

class RialtoClientMediaPipelineIpcDataTest : public MediaPipelineIpcTestBase
{
protected:
    int32_t m_sourceId = 1U;
    uint32_t m_requestId = 2U;
    size_t m_frameCount = 3U;
    uint32_t m_numFrames = 5U;
    std::shared_ptr<MediaPlayerShmInfo> m_shmInfo;

    std::mutex m_notifyMutex;
    std::condition_variable m_notifyCond;

    virtual void SetUp()
    {
        MediaPipelineIpcTestBase::SetUp();

        createMediaPipelineIpc();

        initShmInfo();
    }

    virtual void TearDown()
    {
        destroyMediaPipelineIpc();

        MediaPipelineIpcTestBase::TearDown();
    }

    void initShmInfo()
    {
        m_shmInfo = std::make_shared<MediaPlayerShmInfo>();
        m_shmInfo->maxMetadataBytes = 5;
        m_shmInfo->metadataOffset = 6;
        m_shmInfo->mediaDataOffset = 7;
        m_shmInfo->maxMediaBytes = 4U;
    }

    std::shared_ptr<firebolt::rialto::NeedMediaDataEvent> createNeedDataEvent(bool withShmInfo)
    {
        auto needMediaDataEvent = std::make_shared<firebolt::rialto::NeedMediaDataEvent>();
        needMediaDataEvent->set_session_id(m_sessionId);
        needMediaDataEvent->set_source_id(m_sourceId);
        needMediaDataEvent->set_request_id(m_requestId);
        needMediaDataEvent->set_frame_count(m_frameCount);

        if (withShmInfo)
        {
            auto shmInfoProto = needMediaDataEvent->mutable_shm_info();
            shmInfoProto->set_max_metadata_bytes(m_shmInfo->maxMetadataBytes);
            shmInfoProto->set_metadata_offset(m_shmInfo->metadataOffset);
            shmInfoProto->set_media_data_offset(m_shmInfo->mediaDataOffset);
            shmInfoProto->set_max_media_bytes(m_shmInfo->maxMediaBytes);
        }

        return needMediaDataEvent;
    }
};

/**
 * Test that a need data event over IPC with shminfo is forwarded to the client.
 */
TEST_F(RialtoClientMediaPipelineIpcDataTest, NeedDataWithShmInfo)
{
    auto needMediaDataEvent = createNeedDataEvent(true);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_clientMock, notifyNeedMediaData(m_sourceId, m_frameCount, m_requestId, ShmInfoMatcher(m_shmInfo)));

    m_needDataCb(needMediaDataEvent);
}

/**
 * Test that a need data event over IPC without shminfo is forwarded to the client.
 */
TEST_F(RialtoClientMediaPipelineIpcDataTest, NeedDataNoShmInfo)
{
    auto needMediaDataEvent = createNeedDataEvent(false);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_clientMock, notifyNeedMediaData(m_sourceId, m_frameCount, m_requestId, IsNull()));

    m_needDataCb(needMediaDataEvent);
}

/**
 * Test that if the session id of the event is not the same as the playback session the event will be ignored.
 */
TEST_F(RialtoClientMediaPipelineIpcDataTest, NeedDataInvalidSessionId)
{
    auto needMediaDataEvent = createNeedDataEvent(false);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    needMediaDataEvent->set_session_id(-1);

    m_needDataCb(needMediaDataEvent);
}

/**
 * Test that haveData can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcDataTest, HaveDataSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("haveData"), m_controllerMock.get(),
                           haveDataRequestMatcher(m_sessionId, firebolt::rialto::HaveDataRequest_MediaSourceStatus_OK,
                                                  m_numFrames, m_requestId),
                           _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->haveData(MediaSourceStatus::OK, m_numFrames, m_requestId), true);
}

/**
 * Test that haveData fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcDataTest, HaveDataFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("haveData"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->haveData(MediaSourceStatus::OK, m_numFrames, m_requestId), false);
}

/**
 * Test that HaveData fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcDataTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->haveData(MediaSourceStatus::OK, m_numFrames, m_requestId), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that HaveData fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcDataTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("haveData"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->haveData(MediaSourceStatus::OK, m_numFrames, m_requestId), true);
}
