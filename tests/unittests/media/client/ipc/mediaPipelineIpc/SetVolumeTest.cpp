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

class RialtoClientMediaPipelineIpcSetVolumeTest : public MediaPipelineIpcTestBase
{
protected:
    double m_targetVolume{0.7};
    uint32_t m_volumeDuration{1000};
    firebolt::rialto::EaseType m_easeType{firebolt::rialto::EaseType::EASE_LINEAR};

    virtual void SetUp()
    {
        MediaPipelineIpcTestBase::SetUp();

        createMediaPipelineIpc();
    }

    virtual void TearDown()
    {
        destroyMediaPipelineIpc();

        MediaPipelineIpcTestBase::TearDown();
    }
};

/**
 * Test that setVolume can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSetVolumeTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("setVolume"), m_controllerMock.get(),
                           setVolumeRequestMatcher(m_sessionId, m_targetVolume, m_volumeDuration, m_easeType), _,
                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->setVolume(m_targetVolume, m_volumeDuration, m_easeType), true);
}

/**
 * Test that setVolume fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetVolumeTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->setVolume(m_targetVolume, m_volumeDuration, m_easeType), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setVolume fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSetVolumeTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setVolume"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setVolume(m_targetVolume, m_volumeDuration, m_easeType), true);
}

/**
 * Test that setVolume fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSetVolumeTest, SetVolumeFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setVolume"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->setVolume(m_targetVolume, m_volumeDuration, m_easeType), false);
}
