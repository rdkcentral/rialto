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

#include "GstWebAudioPlayerTestCommon.h"
#include "PlayerTaskMock.h"
#include <thread>

using testing::DoAll;
using testing::WithArgs;

class GstWebAudioPlayerTest : public GstWebAudioPlayerTestCommon
{
protected:
    std::unique_ptr<IGstWebAudioPlayer> m_sut;
    std::thread m_testThread;
    const uint32_t m_bytesWritten{123};

    GstWebAudioPlayerTest()
    {
        gstPlayerWillBeCreatedForGenericPlatform();
        m_sut = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock, m_glibWrapperMock,
                                                    m_gstSrcFactoryMock, std::move(m_taskFactory),
                                                    std::move(workerThreadFactory),
                                                    std::move(gstDispatcherThreadFactory));
    }

    ~GstWebAudioPlayerTest() override
    {
        gstPlayerWillBeDestroyed();
        m_sut.reset();
    }

public:
    void notifyWriteBuffer(WebAudioPlayerContext &context)
    {
        {
            std::unique_lock<std::mutex> lock(context.writeBufferMutex);
            context.lastBytesWritten = m_bytesWritten;
        }
        context.writeBufferCond.notify_one();
    }
};

TEST_F(GstWebAudioPlayerTest, shouldSetCaps)
{
    const std::string audioMimeType{"audio/x-raw"};
    const WebAudioConfig config{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetCaps(_, audioMimeType, &config)).WillOnce(Return(ByMove(std::move(task))));
    executeTaskWhenEnqueued();

    m_sut->setCaps(audioMimeType, &config);
}

TEST_F(GstWebAudioPlayerTest, shouldPlay)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPlay(_)).WillOnce(Return(ByMove(std::move(task))));
    executeTaskWhenEnqueued();

    m_sut->play();
}

TEST_F(GstWebAudioPlayerTest, shouldPause)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPause(_)).WillOnce(Return(ByMove(std::move(task))));
    executeTaskWhenEnqueued();

    m_sut->pause();
}

TEST_F(GstWebAudioPlayerTest, shouldSetEos)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createEos(_)).WillOnce(Return(ByMove(std::move(task))));
    executeTaskWhenEnqueued();

    m_sut->setEos();
}

TEST_F(GstWebAudioPlayerTest, shouldSetVolume)
{
    constexpr double kVolume{0.7};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetVolume(_, kVolume)).WillOnce(Return(ByMove(std::move(task))));
    executeTaskWhenEnqueued();

    m_sut->setVolume(kVolume);
}

TEST_F(GstWebAudioPlayerTest, shouldReturnVolume)
{
    constexpr double kVolume{0.7};
    double resultVolume{};
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeGetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR)).WillOnce(Return(kVolume));
    EXPECT_TRUE(m_sut->getVolume(resultVolume));
    EXPECT_EQ(resultVolume, kVolume);
}

TEST_F(GstWebAudioPlayerTest, writeBufferShouldReturn0OnTimeout)
{
    uint8_t mainPtr{};
    uint32_t mainLength = 1;
    uint8_t wrapPtr{};
    uint32_t wrapLength = 2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createWriteBuffer(_, &mainPtr, mainLength, &wrapPtr, wrapLength))
        .WillOnce(Return(ByMove(std::move(task))));
    executeTaskWhenEnqueued();

    uint32_t returnBytes = m_sut->writeBuffer(&mainPtr, mainLength, &wrapPtr, wrapLength);
    EXPECT_EQ(returnBytes, 0);
}

TEST_F(GstWebAudioPlayerTest, shouldWriteBuffer)
{
    uint8_t mainPtr{};
    uint32_t mainLength = 1;
    uint8_t wrapPtr{};
    uint32_t wrapLength = 2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createWriteBuffer(_, &mainPtr, mainLength, &wrapPtr, wrapLength))
        .WillOnce(DoAll(WithArgs<0>(Invoke(
                            [this](WebAudioPlayerContext &context) {
                                m_testThread =
                                    std::thread(&GstWebAudioPlayerTest::notifyWriteBuffer, this, std::ref(context));
                            })),
                        Return(ByMove(std::move(task)))));
    executeTaskWhenEnqueued();

    uint32_t returnBytes = m_sut->writeBuffer(&mainPtr, mainLength, &wrapPtr, wrapLength);
    EXPECT_EQ(returnBytes, m_bytesWritten);

    m_testThread.join();
}

TEST_F(GstWebAudioPlayerTest, shouldGetQueuedBytes)
{
    constexpr uint64_t kQueuedBytes{4567};
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCurrentLevelBytes(_)).WillOnce(Return(kQueuedBytes));
    EXPECT_EQ(m_sut->getQueuedBytes(), kQueuedBytes);
}
