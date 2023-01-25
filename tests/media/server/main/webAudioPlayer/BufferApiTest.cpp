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

class RialtoServerWebAudioPlayerBufferApiTest : public WebAudioPlayerTestBase
{
protected:
    uint32_t m_availableFrames{12};
    uint32_t m_framesStored{0};
    const std::chrono::milliseconds m_kExpectedWriteDataTimeout{100};
    std::shared_ptr<WebAudioShmInfo> m_webAudioShmInfo;
    std::function<void()> m_writeBufferTimerCallback;

    RialtoServerWebAudioPlayerBufferApiTest()
    {
        createWebAudioPlayer();
    }

    ~RialtoServerWebAudioPlayerBufferApiTest() { destroyWebAudioPlayer(); }

    void getBufferAvailableSuccess(uint32_t expectedAvailableFrames, const std::shared_ptr<WebAudioShmInfo>& expectedShmInfo = nullptr)
    {
        m_webAudioShmInfo = std::make_shared<WebAudioShmInfo>();
        mainThreadWillEnqueueTaskAndWait();
        bool status = m_webAudioPlayer->getBufferAvailable(m_availableFrames, m_webAudioShmInfo);
        EXPECT_EQ(status, true);
        EXPECT_EQ(m_availableFrames, expectedAvailableFrames);
        if (expectedShmInfo)
        {
            EXPECT_EQ(m_webAudioShmInfo->lengthMain, expectedShmInfo->lengthMain);
            EXPECT_EQ(m_webAudioShmInfo->offsetMain, expectedShmInfo->offsetMain);
            EXPECT_EQ(m_webAudioShmInfo->lengthWrap, expectedShmInfo->lengthWrap);
            EXPECT_EQ(m_webAudioShmInfo->offsetWrap, expectedShmInfo->offsetWrap);
        }
        std::cout << "getBufferAvailableSuccess: " << m_webAudioShmInfo->lengthMain << ", " << m_webAudioShmInfo->offsetMain << std::endl;
        std::cout << "getBufferAvailableSuccess: " << m_webAudioShmInfo->lengthWrap << ", " << m_webAudioShmInfo->offsetWrap << std::endl;

    }

    void writeBufferSuccess(uint32_t newFramesToWrite)
    {
        mainThreadWillEnqueueTaskAndWait();
        bool status = m_webAudioPlayer->writeBuffer(newFramesToWrite, nullptr);
        EXPECT_EQ(status, true);
    }

    void expectWriteStoredFrames(uint32_t storedFramesToWrite, uint32_t storedFramesWritten)
    {
        uint8_t *expectedStoredMainPtr = nullptr;
        uint8_t *expectedStoredWrapPtr = nullptr;
        uint32_t expectedStoredMainLength = 0;
        uint32_t expectedStoredWrapLength = 0;

        if (m_webAudioShmInfo->lengthWrap != 0)
        {
            // Should start reading data from the end of the wrap free buffer
            expectedStoredMainPtr = &m_dataPtr + m_webAudioShmInfo->offsetWrap + m_webAudioShmInfo->lengthWrap;
            expectedStoredMainLength = storedFramesToWrite * 4;
        }
        else
        {
            // Should start reading data from the end of the main free buffer
            expectedStoredMainPtr = &m_dataPtr + m_webAudioShmInfo->offsetMain + m_webAudioShmInfo->lengthMain;
            std::cout << m_dataLen << ", " << m_webAudioShmInfo->offsetMain << ", " << m_webAudioShmInfo->lengthMain << std::endl;
            expectedStoredMainLength = m_dataLen - (m_webAudioShmInfo->offsetMain + m_webAudioShmInfo->lengthMain);
            // Continue reading data from the start of the buffer
            expectedStoredWrapPtr = &m_dataPtr;
            expectedStoredWrapLength = m_webAudioShmInfo->offsetMain - m_webAudioShmInfo->offsetWrap;
        }

        std::cout << expectedStoredMainLength << ", " << expectedStoredWrapLength << std::endl;
        EXPECT_CALL(*m_gstPlayerMock, writeBuffer(expectedStoredMainPtr, expectedStoredMainLength, expectedStoredWrapPtr, expectedStoredWrapLength)).WillOnce(Return(storedFramesWritten * 4)).RetiresOnSaturation();

        m_framesStored += - storedFramesWritten;
    }

    void expectWriteNewFrames(uint32_t newFramesToWrite, uint32_t newFramesWritten)
    {
        uint8_t *expectedNewMainPtr = &m_dataPtr + m_webAudioShmInfo->offsetMain;
        uint8_t *expectedNewWrapPtr = &m_dataPtr + m_webAudioShmInfo->offsetWrap;
        uint32_t expectedNewMainLength = 0;
        uint32_t expectedNewWrapLength = 0;

        if (newFramesToWrite * 4 <= m_webAudioShmInfo->lengthMain)
        {
            expectedNewMainLength = newFramesToWrite * 4;
        }
        else
        {
            expectedNewMainLength = m_webAudioShmInfo->lengthMain;
            expectedNewWrapLength = newFramesToWrite * 4 - m_webAudioShmInfo->lengthMain;
        }

        EXPECT_CALL(*m_gstPlayerMock, writeBuffer(expectedNewMainPtr, expectedNewMainLength, expectedNewWrapPtr, expectedNewWrapLength)).WillOnce(Return(newFramesWritten * 4)).RetiresOnSaturation();

        m_framesStored += newFramesToWrite - newFramesWritten;
    }

    void expectStartTimer()
    {
        m_timer = std::make_unique<StrictMock<TimerMock>>();
        m_timerMock = static_cast<StrictMock<TimerMock> *>(m_timer.get());
        EXPECT_CALL(*m_timerFactoryMock, createTimer(m_kExpectedWriteDataTimeout, _, _))
            .WillOnce(DoAll(SaveArg<1>(&m_writeBufferTimerCallback), Return(ByMove(std::move(m_timer))))).RetiresOnSaturation();
    }

    void expectCancelTimer()
    {
        EXPECT_CALL(*m_timerMock, isActive()).WillOnce(Return(true)).RetiresOnSaturation();
        EXPECT_CALL(*m_timerMock, cancel()).RetiresOnSaturation();
    }

    void writeBufferFailure(uint32_t framesWritten)
    {
        mainThreadWillEnqueueTaskAndWait();
        bool status = m_webAudioPlayer->writeBuffer(framesWritten, nullptr);
        EXPECT_EQ(status, false);
    }
};

/**
 * Test that getDeviceInfo returns the correct device information.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, getDeviceInfo)
{
    //destroyWebAudioPlayer();

    uint32_t returnPreferredFrames{};
    uint32_t returnMaximumFrames{};
    bool returnSupportDeferredPlay{};

    bool status = m_webAudioPlayer->getDeviceInfo(returnPreferredFrames, returnMaximumFrames, returnSupportDeferredPlay);
    EXPECT_EQ(status, true);
    EXPECT_EQ(640, returnPreferredFrames);
    EXPECT_EQ(m_dataLen / 4, returnMaximumFrames);
    EXPECT_EQ(true, returnSupportDeferredPlay);
}

/**
 * Test that getDeviceInfo returns the correct device information, if the shm is small and cannot fit the PreferredFrames in it
 * getDeviceInfo shall return maximumFrames.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, getDeviceInfoSmallSharedBuffer)
{
    // Destroy player with large shared memory buffer and create a new one with smaller buffer
    destroyWebAudioPlayer();
    m_dataLen = 80;
    m_gstPlayer = std::make_unique<StrictMock<GstWebAudioPlayerMock>>();
    m_gstPlayerMock = static_cast<StrictMock<GstWebAudioPlayerMock> *>(m_gstPlayer.get());
    createWebAudioPlayer();

    uint32_t returnPreferredFrames{};
    uint32_t returnMaximumFrames{};
    bool returnSupportDeferredPlay{};

    bool status = m_webAudioPlayer->getDeviceInfo(returnPreferredFrames, returnMaximumFrames, returnSupportDeferredPlay);
    EXPECT_EQ(status, true);
    EXPECT_EQ(m_dataLen / 4, returnPreferredFrames);
    EXPECT_EQ(m_dataLen / 4, returnMaximumFrames);
    EXPECT_EQ(true, returnSupportDeferredPlay);
}

/**
 * Test that getBufferAvailable returns available frames and shm information after construction of the
 * WebAudioPlayer only.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, initialGetBufferAvailable)
{
    uint32_t returnAvailableFrames{};
    std::shared_ptr<WebAudioShmInfo> returnWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();

    mainThreadWillEnqueueTaskAndWait();

    // After constuction of the object the available frames should be set to the maximum and the shmInfo should be
    // the default.
    bool status = m_webAudioPlayer->getBufferAvailable(returnAvailableFrames, returnWebAudioShmInfo);
    EXPECT_EQ(status, true);
    EXPECT_EQ(m_dataLen / 4, returnAvailableFrames);
    EXPECT_EQ(m_dataLen, returnWebAudioShmInfo->lengthMain);
    EXPECT_EQ(0, returnWebAudioShmInfo->offsetMain);
    EXPECT_EQ(0, returnWebAudioShmInfo->lengthWrap);
    EXPECT_EQ(0, returnWebAudioShmInfo->offsetWrap);
}

/**
 * Test that writeBuffer successfully writes the first buffer to gstreamer.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, initialWriteBuffer)
{
    // getBufferAvailable must be called before a writeBuffer call
    getBufferAvailableSuccess(m_dataLen /  4);

    expectWriteNewFrames(m_availableFrames, m_availableFrames);
    writeBufferSuccess(m_availableFrames);
}

/**
 * Test that writeBuffer fails if there was no getBufferAvailable call previous.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeBufferWithNoGetBufferAvailable)
{
    writeBufferFailure(123);
}

/**
 * Test that writeBuffer only succeeds once per getBufferAvailable call.
 * There is no previously stored data.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, multipleWriteBufferWithOneGetBufferAvailableNoStoredData)
{
    getBufferAvailableSuccess(m_dataLen /  4);

    expectWriteNewFrames(m_availableFrames / 2, m_availableFrames / 2);
    writeBufferSuccess(m_availableFrames / 2);

    writeBufferFailure(m_availableFrames / 2);

    writeBufferFailure(m_availableFrames / 2);
}

/**
 * Test that subsequent getBufferAvailable calls overwrite the previous if a writeBuffer has not been called.
 * There is no previously stored data.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, multipleGetBufferAvailableNoStoredData)
{
    getBufferAvailableSuccess(m_dataLen /  4);

    getBufferAvailableSuccess(m_dataLen /  4);

    getBufferAvailableSuccess(m_dataLen /  4);

    expectWriteNewFrames(m_availableFrames / 2, m_availableFrames / 2);
    writeBufferSuccess(m_availableFrames / 2);

    writeBufferFailure(m_availableFrames / 2);
}

/**
 * Test that available buffer returns no space if all data is stored in the shared memory.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, fullSharedMemory)
{
    getBufferAvailableSuccess(m_dataLen /  4);
    expectWriteNewFrames(m_availableFrames, 0);
    expectStartTimer();
    writeBufferSuccess(m_availableFrames);

    uint32_t returnAvailableFrames{};
    std::shared_ptr<WebAudioShmInfo> returnWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();

    mainThreadWillEnqueueTaskAndWait();

    bool status = m_webAudioPlayer->getBufferAvailable(returnAvailableFrames, returnWebAudioShmInfo);
    EXPECT_EQ(status, true);
    EXPECT_EQ(0, returnAvailableFrames);
    EXPECT_EQ(0, returnWebAudioShmInfo->lengthMain);
    EXPECT_EQ(0, returnWebAudioShmInfo->offsetMain);
    EXPECT_EQ(0, returnWebAudioShmInfo->lengthWrap);
    EXPECT_EQ(0, returnWebAudioShmInfo->offsetWrap);

    expectCancelTimer();
}

/**
 * Test that the stored data is successfully written to gstreamer but new data is not if no new frames written.
 * The data stored in the shared memory has not wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeAllStoredDataAndNoNewData)
{
    getBufferAvailableSuccess(m_dataLen /  4);
    uint32_t framesToWrite = m_availableFrames;
    uint32_t framesWritten = m_availableFrames / 2;
    expectWriteNewFrames(framesToWrite, framesWritten);
    expectStartTimer();
    writeBufferSuccess(framesToWrite);

    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = framesWritten * 4;
    expectedWebAudioShmInfo->offsetMain = 0;
    expectedWebAudioShmInfo->lengthWrap = 0;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(framesWritten, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    expectCancelTimer();
    writeBufferSuccess(0);
}

/**
 * Test that the stored data is successfully written to gstreamer before writting the new data.
 * The data stored in the shared memory has not wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeAllStoredDataAndNewData)
{
    getBufferAvailableSuccess(m_dataLen /  4);
    uint32_t framesToWrite = m_availableFrames;
    uint32_t framesWritten = m_availableFrames / 2;
    expectWriteNewFrames(framesToWrite, framesWritten);
    expectStartTimer();
    writeBufferSuccess(framesToWrite);

    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = framesWritten * 4;
    expectedWebAudioShmInfo->offsetMain = 0;
    expectedWebAudioShmInfo->lengthWrap = 0;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(framesWritten, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    expectWriteNewFrames(framesWritten, framesWritten);
    expectCancelTimer();
    writeBufferSuccess(framesWritten);
}

/**
 * Test that the stored data is successfully written to gstreamer but new data is not if not all data has been written.
 * The data stored in the shared memory has not wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writePartialStoredDataAndNoNewData)
{
    uint32_t maxFramesToWrite = m_dataLen /  4;

    // Store some data in the shared memory and dont write
    getBufferAvailableSuccess(m_dataLen /  4);
    expectWriteNewFrames(maxFramesToWrite / 2, maxFramesToWrite / 4);
    expectStartTimer();
    writeBufferSuccess(maxFramesToWrite / 2);

    // Do not write all the stored data
    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = (maxFramesToWrite / 2) *4;
    expectedWebAudioShmInfo->offsetMain = (maxFramesToWrite / 2) *4;
    expectedWebAudioShmInfo->lengthWrap = (maxFramesToWrite / 4) *4;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(maxFramesToWrite - m_framesStored, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored / 2);
    expectCancelTimer();
    expectStartTimer();
    m_framesStored += maxFramesToWrite / 4;
    writeBufferSuccess(maxFramesToWrite / 4);

    // Check available buffer correct
    expectedWebAudioShmInfo->lengthMain = (maxFramesToWrite / 4) *4;
    expectedWebAudioShmInfo->offsetMain = (maxFramesToWrite  *3 / 4) *4;
    expectedWebAudioShmInfo->lengthWrap = (maxFramesToWrite / 4 + maxFramesToWrite / 8) *4;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(maxFramesToWrite - m_framesStored, expectedWebAudioShmInfo);

    expectCancelTimer();
}

/**
 * Test that the stored data and new data is successfully written to gstreamer.
 * The shared data stored in the shared memory has wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeAllStoredWrappedDataAndNewData)
{
    uint32_t maxFramesToWrite = m_dataLen /  4;

    // Store data in a quater of shared memory buffer at the end of the buffer
    getBufferAvailableSuccess(m_dataLen /  4);
    expectWriteNewFrames(maxFramesToWrite, maxFramesToWrite*3/4);
    expectStartTimer();
    writeBufferSuccess(maxFramesToWrite);

    // Store data in a quater of shared memory buffer at the start of the buffer
    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = (maxFramesToWrite*3/4) *4;
    expectedWebAudioShmInfo->offsetMain = 0;
    expectedWebAudioShmInfo->lengthWrap = 0;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(maxFramesToWrite*3/4, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, 0);
    expectCancelTimer();
    expectStartTimer();
    m_framesStored += maxFramesToWrite / 4;
    writeBufferSuccess(maxFramesToWrite / 4);

    // Write all stored data and new data
    expectedWebAudioShmInfo->lengthMain = (maxFramesToWrite/2) *4;
    expectedWebAudioShmInfo->offsetMain = (maxFramesToWrite/4) *4;
    expectedWebAudioShmInfo->lengthWrap = 0;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(maxFramesToWrite - m_framesStored, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    expectWriteNewFrames(maxFramesToWrite / 4, maxFramesToWrite / 4);
    expectCancelTimer();
    writeBufferSuccess(maxFramesToWrite / 4);

    // Check buffer now empty
    expectedWebAudioShmInfo->lengthMain = (maxFramesToWrite/2) *4;
    expectedWebAudioShmInfo->offsetMain = (maxFramesToWrite/4) *4;
    expectedWebAudioShmInfo->lengthWrap = 0;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(m_dataLen /  4);
}

/**
 * Test that the stored data and new data is successfully written to gstreamer.
 * The new data stored in the shared memory has wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeAllStoredDataAndWrappedNewData)
{
    uint32_t maxFramesToWrite = m_dataLen /  4;

    // Store data in the 3rd quater of the shared memory
    getBufferAvailableSuccess(m_dataLen /  4);
    expectWriteNewFrames(maxFramesToWrite*3/4, maxFramesToWrite/2);
    expectStartTimer();
    writeBufferSuccess(maxFramesToWrite*3/4);

    // Write data that wraps the shared memory
    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = (maxFramesToWrite/4) *4;
    expectedWebAudioShmInfo->offsetMain = (maxFramesToWrite*3/4) *4;
    expectedWebAudioShmInfo->lengthWrap = (maxFramesToWrite/2)*4;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(maxFramesToWrite - m_framesStored, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    expectWriteNewFrames(maxFramesToWrite/2, maxFramesToWrite/2);
    expectCancelTimer();
    writeBufferSuccess(maxFramesToWrite/2);

    // Check buffer now empty
    expectedWebAudioShmInfo->lengthMain = (maxFramesToWrite/2) *4;
    expectedWebAudioShmInfo->offsetMain = (maxFramesToWrite/4) *4;
    expectedWebAudioShmInfo->lengthWrap = 0;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(m_dataLen /  4);
}

/**
 * Test that the stored data is written on the writeBufferTimer.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, handleWriteBufferTimerAllData)
{
    getBufferAvailableSuccess(m_dataLen /  4);
    expectWriteNewFrames(m_availableFrames, 0);
    expectStartTimer();
    writeBufferSuccess(m_availableFrames);

    getBufferAvailableSuccess(0);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    mainThreadWillEnqueueTask();
    m_writeBufferTimerCallback();
}

/**
 * Test that the stored data is written on the writeBufferTimer, and the timer is restarted if all data could not be written.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, handleWriteBufferTimerPartialData)
{
    getBufferAvailableSuccess(m_dataLen /  4);
    expectWriteNewFrames(m_availableFrames, 0);
    expectStartTimer();
    writeBufferSuccess(m_availableFrames);

    getBufferAvailableSuccess(0);
    expectWriteStoredFrames(m_framesStored, m_framesStored / 2);
    expectStartTimer();
    mainThreadWillEnqueueTask();
    m_writeBufferTimerCallback();

    expectCancelTimer();
}
