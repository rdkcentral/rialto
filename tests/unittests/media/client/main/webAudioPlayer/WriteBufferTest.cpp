/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "SharedMemoryHandleMock.h"
#include "WebAudioPlayerTestBase.h"

#include <vector>

class RialtoClientWebAudioPlayerWriteBufferTest : public WebAudioPlayerTestBase
{
protected:
    uint32_t m_numberOfFrames{1};
    std::vector<uint8_t> m_dataSrc{1, 2, 3, 4, 55, 66, 77, 88};
    std::vector<uint8_t> m_dataDest{0, 0, 0, 0, 0, 0, 0, 0};
    std::shared_ptr<WebAudioShmInfo> m_webAudioShmInfo;
    std::shared_ptr<SharedMemoryHandleMock> m_sharedMemoryHandleMock;

    virtual void SetUp()
    {
        WebAudioPlayerTestBase::SetUp();
        m_webAudioShmInfo = std::make_shared<WebAudioShmInfo>();
        m_sharedMemoryHandleMock = std::make_shared<SharedMemoryHandleMock>();

        createWebAudioPlayer();
    }

    virtual void TearDown()
    {
        destroyWebAudioPlayer();
        m_webAudioShmInfo.reset();
        m_sharedMemoryHandleMock.reset();
        WebAudioPlayerTestBase::TearDown();
    }
};

TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, webAudoSharedInfoIsNullError)
{
    EXPECT_FALSE(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_dataSrc.data()));
}

TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, numberOfFramesToWriteExceedsTheAvailableSpaceError)
{
    std::shared_ptr<WebAudioShmInfo> notUsedWebAudioShmInfo;
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(_, _))
        .WillOnce(Invoke(
            [this](uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
            {
                webAudioShmInfo->offsetMain = 0;
                webAudioShmInfo->lengthMain = 0;
                return true;
            }));
    m_webAudioPlayer->getBufferAvailable(m_numberOfFrames, notUsedWebAudioShmInfo);

    EXPECT_FALSE(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_dataSrc.data()));
}

TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, clientInInactiveState)
{
    std::shared_ptr<WebAudioShmInfo> notUsedWebAudioShmInfo;
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(_, _))
        .WillOnce(Invoke(
            [this](uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
            {
                webAudioShmInfo->offsetMain = 0;
                webAudioShmInfo->lengthMain = 4;
                return true;
            }));
    m_webAudioPlayer->getBufferAvailable(m_numberOfFrames, notUsedWebAudioShmInfo);

    m_webAudioPlayer->notifyApplicationState(ApplicationState::INACTIVE);

    EXPECT_FALSE(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_dataSrc.data()));
}

TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, sharedBufferNoLongerValidError)
{
    std::shared_ptr<WebAudioShmInfo> notUsedWebAudioShmInfo;
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(_, _))
        .WillOnce(Invoke(
            [this](uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
            {
                webAudioShmInfo->offsetMain = 0;
                webAudioShmInfo->lengthMain = 4;
                return true;
            }));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle()).WillOnce(Invoke([this]() { return nullptr; }));
    m_webAudioPlayer->getBufferAvailable(m_numberOfFrames, notUsedWebAudioShmInfo);

    EXPECT_FALSE(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_dataSrc.data()));
}

TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, sharedBufferShmNotValidError)
{
    std::shared_ptr<WebAudioShmInfo> notUsedWebAudioShmInfo;
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(_, _))
        .WillOnce(Invoke(
            [this](uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
            {
                webAudioShmInfo->offsetMain = 0;
                webAudioShmInfo->lengthMain = 4;
                return true;
            }));
    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle())
        .WillOnce(Invoke([this]() { return m_sharedMemoryHandleMock; }));
    m_webAudioPlayer->getBufferAvailable(m_numberOfFrames, notUsedWebAudioShmInfo);

    EXPECT_FALSE(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_dataSrc.data()));
}

TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, writeBufferIpcCallFailsError)
{
    std::shared_ptr<WebAudioShmInfo> notUsedWebAudioShmInfo;
    EXPECT_CALL(*m_webAudioPlayerIpcMock, writeBuffer(m_numberOfFrames)).WillOnce(Return(false));
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(_, _))
        .WillOnce(Invoke(
            [this](uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
            {
                webAudioShmInfo->offsetMain = 0;
                webAudioShmInfo->lengthMain = 4;
                return true;
            }));
    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(m_dataDest.data()));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle())
        .WillOnce(Invoke([this]() { return m_sharedMemoryHandleMock; }));
    m_webAudioPlayer->getBufferAvailable(m_numberOfFrames, notUsedWebAudioShmInfo);

    EXPECT_FALSE(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_dataSrc.data()));
}

TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, writeToMainOffsetOnly)
{
    std::shared_ptr<WebAudioShmInfo> notUsedWebAudioShmInfo;
    EXPECT_CALL(*m_webAudioPlayerIpcMock, writeBuffer(m_numberOfFrames)).WillOnce(Return(true));
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(_, _))
        .WillOnce(Invoke(
            [this](uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
            {
                availableFrames = 1;
                webAudioShmInfo->offsetMain = 0;
                webAudioShmInfo->lengthMain = 4;
                webAudioShmInfo->offsetWrap = 1000;
                webAudioShmInfo->lengthWrap = 1000;
                return true;
            }));
    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(m_dataDest.data()));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle())
        .WillOnce(Invoke([this]() { return m_sharedMemoryHandleMock; }));
    m_webAudioPlayer->getBufferAvailable(m_numberOfFrames, notUsedWebAudioShmInfo);
    EXPECT_EQ(1, m_numberOfFrames);

    EXPECT_EQ(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_dataSrc.data()), true);
    EXPECT_EQ(m_dataSrc[0], m_dataDest[0]);
    EXPECT_EQ(m_dataSrc[1], m_dataDest[1]);
    EXPECT_EQ(m_dataSrc[2], m_dataDest[2]);
    EXPECT_EQ(m_dataSrc[3], m_dataDest[3]);
    EXPECT_EQ(m_dataDest[4], 0);
}

TEST_F(RialtoClientWebAudioPlayerWriteBufferTest, writeToMainAndWrapOffset)
{
    std::shared_ptr<WebAudioShmInfo> notUsedWebAudioShmInfo;
    EXPECT_CALL(*m_webAudioPlayerIpcMock, writeBuffer(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_webAudioPlayerIpcMock, getBufferAvailable(_, _))
        .WillOnce(Invoke(
            [this](uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
            {
                availableFrames = 2;
                webAudioShmInfo->offsetMain = 4;
                webAudioShmInfo->lengthMain = 4;
                webAudioShmInfo->offsetWrap = 0;
                webAudioShmInfo->lengthWrap = 4;
                return true;
            }));
    EXPECT_CALL(*m_sharedMemoryHandleMock, getShm()).WillRepeatedly(Return(m_dataDest.data()));
    EXPECT_CALL(*m_clientControllerMock, getSharedMemoryHandle())
        .WillOnce(Invoke([this]() { return m_sharedMemoryHandleMock; }));
    m_webAudioPlayer->getBufferAvailable(m_numberOfFrames, notUsedWebAudioShmInfo);
    EXPECT_EQ(2, m_numberOfFrames);

    EXPECT_EQ(m_webAudioPlayer->writeBuffer(m_numberOfFrames, m_dataSrc.data()), true);
    EXPECT_EQ(m_dataSrc[0], m_dataDest[4]);
    EXPECT_EQ(m_dataSrc[1], m_dataDest[5]);
    EXPECT_EQ(m_dataSrc[2], m_dataDest[6]);
    EXPECT_EQ(m_dataSrc[3], m_dataDest[7]);
    EXPECT_EQ(m_dataSrc[4], m_dataDest[0]);
    EXPECT_EQ(m_dataSrc[5], m_dataDest[1]);
    EXPECT_EQ(m_dataSrc[6], m_dataDest[2]);
    EXPECT_EQ(m_dataSrc[7], m_dataDest[3]);
}
