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

#include "WebAudioPlayerTestBase.h"

class RialtoServerWebAudioPlayerMiscellaneousFunctionsTest : public WebAudioPlayerTestBase
{
protected:
    std::mutex m_apiMutex;
    std::condition_variable m_apiCond;
    bool m_threadDone{false};
    const double m_volume{123};

    RialtoServerWebAudioPlayerMiscellaneousFunctionsTest() { createWebAudioPlayer(); }

    ~RialtoServerWebAudioPlayerMiscellaneousFunctionsTest() { destroyWebAudioPlayer(); }

public:
    void notifyApiCalled()
    {
        std::unique_lock<std::mutex> lock(m_apiMutex);
        m_threadDone = true;
        m_apiCond.notify_all();
    }

    void waitForApiCalled()
    {
        std::unique_lock<std::mutex> lock(m_apiMutex);
        bool status = m_apiCond.wait_for(lock, std::chrono::milliseconds(200), [this]() { return m_threadDone; });
        EXPECT_TRUE(status);
    }
};

/**
 * Test that play triggers a play on the WebAudioGstPlayer on the main thread.
 */
TEST_F(RialtoServerWebAudioPlayerMiscellaneousFunctionsTest, play)
{
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_gstPlayerMock, play())
        .WillOnce(Invoke(this, &RialtoServerWebAudioPlayerMiscellaneousFunctionsTest::notifyApiCalled));

    m_webAudioPlayer->play();

    // Wait for play on main thread
    waitForApiCalled();
}

/**
 * Test that pause triggers a pause on the WebAudioGstPlayer on the main thread.
 */
TEST_F(RialtoServerWebAudioPlayerMiscellaneousFunctionsTest, pause)
{
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_gstPlayerMock, pause())
        .WillOnce(Invoke(this, &RialtoServerWebAudioPlayerMiscellaneousFunctionsTest::notifyApiCalled));

    m_webAudioPlayer->pause();

    // Wait for pause on main thread
    waitForApiCalled();
}

/**
 * Test that setEos triggers a setEos on the WebAudioGstPlayer on the main thread if there are no bytes queued in the shm.
 */
TEST_F(RialtoServerWebAudioPlayerMiscellaneousFunctionsTest, setEos)
{
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_gstPlayerMock, setEos())
        .WillOnce(Invoke(this, &RialtoServerWebAudioPlayerMiscellaneousFunctionsTest::notifyApiCalled));

    m_webAudioPlayer->setEos();

    // Wait for setEos on main thread
    waitForApiCalled();
}

/**
 * Test that setEos does not trigger a setEos on the WebAudioGstPlayer on the main thread if there are queued bytes.
 * setEos then triggered once all the data has been written to gstreamer.
 */
TEST_F(RialtoServerWebAudioPlayerMiscellaneousFunctionsTest, setEosDelayed)
{
    // Fill the shared memory with data
    getBufferAvailableSuccess(m_maxFrame);
    expectWriteNewFrames(m_availableFrames, 0);
    expectStartTimer();
    writeBufferSuccess(m_availableFrames);

    // setEos
    mainThreadWillEnqueueTask();
    m_webAudioPlayer->setEos();

    // Write the stored frames
    getBufferAvailableSuccess(0);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    expectCancelTimer();
    EXPECT_CALL(*m_gstPlayerMock, setEos());
    writeBufferSuccess(0);
}

/**
 * Test that setVolume triggers a setVolume on the WebAudioGstPlayer on the main thread.
 */
TEST_F(RialtoServerWebAudioPlayerMiscellaneousFunctionsTest, setVolume)
{
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_gstPlayerMock, setVolume(m_volume))
        .WillOnce(Invoke(this, &RialtoServerWebAudioPlayerMiscellaneousFunctionsTest::notifyApiCalled));

    m_webAudioPlayer->setVolume(m_volume);

    // Wait for setVolume on main thread
    waitForApiCalled();
}

/**
 * Test that getVolume triggers a getVolume on the WebAudioGstPlayer on the main thread and returns the correct volume.
 */
TEST_F(RialtoServerWebAudioPlayerMiscellaneousFunctionsTest, getVolumeSuccess)
{
    double returnVolume = 0;
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getVolume(_)).WillOnce(DoAll(SetArgReferee<0>(m_volume), Return(true)));

    bool status = m_webAudioPlayer->getVolume(returnVolume);
    EXPECT_EQ(status, true);
    EXPECT_EQ(returnVolume, m_volume);
}

/**
 * Test that getVolume returns error when WebAudioGstPlayer getVolume fails.
 */
TEST_F(RialtoServerWebAudioPlayerMiscellaneousFunctionsTest, getVolumeFailure)
{
    double returnVolume = 0;
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getVolume(_)).WillOnce(Return(false));

    bool status = m_webAudioPlayer->getVolume(returnVolume);
    EXPECT_EQ(status, false);
}
