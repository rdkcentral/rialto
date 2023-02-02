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
    RialtoServerWebAudioPlayerBufferApiTest() { createWebAudioPlayer(); }

    ~RialtoServerWebAudioPlayerBufferApiTest() { destroyWebAudioPlayer(); }

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
    // destroyWebAudioPlayer();

    uint32_t returnPreferredFrames{};
    uint32_t returnMaximumFrames{};
    bool returnSupportDeferredPlay{};

    bool status = m_webAudioPlayer->getDeviceInfo(returnPreferredFrames, returnMaximumFrames, returnSupportDeferredPlay);
    EXPECT_EQ(status, true);
    EXPECT_EQ(640, returnPreferredFrames);
    EXPECT_EQ(m_maxFrame, returnMaximumFrames);
    EXPECT_EQ(true, returnSupportDeferredPlay);
}

/**
 * Test that getDeviceInfo returns the correct device information, if the shm is small and cannot fit the
 * PreferredFrames in it getDeviceInfo shall return maximumFrames.
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
    EXPECT_EQ(m_dataLen / m_bytesPerFrame, returnPreferredFrames);
    EXPECT_EQ(m_dataLen / m_bytesPerFrame, returnMaximumFrames);
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
    EXPECT_EQ(m_maxFrame, returnAvailableFrames);
    EXPECT_EQ(m_dataLen, returnWebAudioShmInfo->lengthMain);
    EXPECT_EQ(m_dataOffset, returnWebAudioShmInfo->offsetMain);
    EXPECT_EQ(0, returnWebAudioShmInfo->lengthWrap);
    EXPECT_EQ(m_dataOffset, returnWebAudioShmInfo->offsetWrap);
}

/**
 * Test that writeBuffer successfully writes the first buffer to gstreamer.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, initialWriteBuffer)
{
    // getBufferAvailable must be called before a writeBuffer call
    getBufferAvailableSuccess(m_maxFrame);

    expectWriteNewFrames(m_availableFrames, m_availableFrames);
    writeBufferSuccess(m_availableFrames);
}

/**
 * Test that getBufferDelay successfully returns 0 if no frames have been written.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, initialGetBufferDelay)
{
    uint32_t returnDelayFrames{};

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getQueuedBytes()).WillOnce(Return(0));

    bool status = m_webAudioPlayer->getBufferDelay(returnDelayFrames);
    EXPECT_EQ(status, true);
    EXPECT_EQ(0, returnDelayFrames);
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
    getBufferAvailableSuccess(m_maxFrame);

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
    getBufferAvailableSuccess(m_maxFrame);

    getBufferAvailableSuccess(m_maxFrame);

    getBufferAvailableSuccess(m_maxFrame);

    expectWriteNewFrames(m_availableFrames / 2, m_availableFrames / 2);
    writeBufferSuccess(m_availableFrames / 2);

    writeBufferFailure(m_availableFrames / 2);
}

/**
 * Test that available buffer returns no space if all data is stored in the shared memory.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, fullSharedMemory)
{
    getBufferAvailableSuccess(m_maxFrame);
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
    EXPECT_EQ(m_dataOffset + m_dataLen, returnWebAudioShmInfo->offsetMain);
    EXPECT_EQ(0, returnWebAudioShmInfo->lengthWrap);
    EXPECT_EQ(m_dataOffset, returnWebAudioShmInfo->offsetWrap);

    expectCancelTimer();
}

/**
 * Test that the stored data is successfully written to gstreamer but new data is not if no new frames written.
 * The data stored in the shared memory has not wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeAllStoredDataAndNoNewData)
{
    getBufferAvailableSuccess(m_maxFrame);
    uint32_t framesToWrite = m_availableFrames;
    uint32_t framesWritten = m_availableFrames / 2;
    expectWriteNewFrames(framesToWrite, framesWritten);
    expectStartTimer();
    writeBufferSuccess(framesToWrite);

    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = 0;
    expectedWebAudioShmInfo->offsetMain = m_dataOffset + m_dataLen;
    expectedWebAudioShmInfo->lengthWrap = framesWritten * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetWrap = m_dataOffset;
    getBufferAvailableSuccess(framesWritten, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    expectCancelTimer();
    writeBufferSuccess(0);

    // Check available buffer correct
    expectedWebAudioShmInfo->lengthMain = 0;
    expectedWebAudioShmInfo->offsetMain = m_dataOffset + m_dataLen;
    expectedWebAudioShmInfo->lengthWrap = m_maxFrame * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetWrap = m_dataOffset;
    getBufferAvailableSuccess(m_maxFrame, expectedWebAudioShmInfo);
}

/**
 * Test that the stored data is successfully written to gstreamer before writting the new data.
 * The data stored in the shared memory has not wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeAllStoredDataAndNewData)
{
    getBufferAvailableSuccess(m_maxFrame);
    uint32_t framesToWrite = m_availableFrames;
    uint32_t framesWritten = m_availableFrames / 2;
    expectWriteNewFrames(framesToWrite, framesWritten);
    expectStartTimer();
    writeBufferSuccess(framesToWrite);

    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = 0;
    expectedWebAudioShmInfo->offsetMain = m_dataOffset + m_dataLen;
    expectedWebAudioShmInfo->lengthWrap = framesWritten * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetWrap = m_dataOffset;
    getBufferAvailableSuccess(framesWritten, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    expectWriteNewFrames(framesWritten, framesWritten);
    expectCancelTimer();
    writeBufferSuccess(framesWritten);

    // Check available buffer correct
    expectedWebAudioShmInfo->lengthMain = m_dataLen / 2;
    expectedWebAudioShmInfo->offsetMain = m_dataOffset + m_dataLen / 2;
    expectedWebAudioShmInfo->lengthWrap = m_dataLen / 2;
    expectedWebAudioShmInfo->offsetWrap = m_dataOffset;
    getBufferAvailableSuccess(m_maxFrame, expectedWebAudioShmInfo);
}

/**
 * Test that the stored data is successfully written to gstreamer but new data is not if not all data has been written.
 * The data stored in the shared memory has not wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writePartialStoredDataAndNoNewData)
{
    // Store some data in the shared memory and dont write
    getBufferAvailableSuccess(m_maxFrame);
    expectWriteNewFrames(m_maxFrame / 2, m_maxFrame / 4);
    expectStartTimer();
    writeBufferSuccess(m_maxFrame / 2);

    // Do not write all the stored data
    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = (m_maxFrame / 2) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetMain = (m_maxFrame / 2) * m_bytesPerFrame + m_dataOffset;
    expectedWebAudioShmInfo->lengthWrap = (m_maxFrame / 4) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetWrap = m_dataOffset;
    getBufferAvailableSuccess(m_maxFrame - m_framesStored, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored / 2);
    expectCancelTimer();
    expectStartTimer();
    m_framesStored += m_maxFrame / 4;
    writeBufferSuccess(m_maxFrame / 4);

    // Check available buffer correct
    expectedWebAudioShmInfo->lengthMain = (m_maxFrame / 4) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetMain = (m_maxFrame * 3 / 4) * m_bytesPerFrame + m_dataOffset;
    expectedWebAudioShmInfo->lengthWrap = (m_maxFrame / 4 + m_maxFrame / 8) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetWrap =  m_dataOffset;
    getBufferAvailableSuccess(m_maxFrame - m_framesStored, expectedWebAudioShmInfo);

    expectCancelTimer();
}

/**
 * Test that the stored data and new data is successfully written to gstreamer.
 * The shared data stored in the shared memory has wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeAllStoredWrappedDataAndNewData)
{
    // Store data in a quater of shared memory buffer at the end of the buffer
    getBufferAvailableSuccess(m_maxFrame);
    expectWriteNewFrames(m_maxFrame, m_maxFrame * 3 / 4);
    expectStartTimer();
    writeBufferSuccess(m_maxFrame);

    // Store data in a quater of shared memory buffer at the start of the buffer
    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = 0;
    expectedWebAudioShmInfo->offsetMain = m_dataOffset + m_dataLen;
    expectedWebAudioShmInfo->lengthWrap = (m_maxFrame * 3 / 4) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetWrap = m_dataOffset;
    getBufferAvailableSuccess(m_maxFrame * 3 / 4, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, 0);
    expectCancelTimer();
    expectStartTimer();
    m_framesStored += m_maxFrame / 4;
    writeBufferSuccess(m_maxFrame / 4);

    // Write all stored data and new data
    expectedWebAudioShmInfo->lengthMain = (m_maxFrame / 2) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetMain = (m_maxFrame / 4) * m_bytesPerFrame + m_dataOffset;
    expectedWebAudioShmInfo->lengthWrap = 0;
    expectedWebAudioShmInfo->offsetWrap = m_dataOffset;
    getBufferAvailableSuccess(m_maxFrame - m_framesStored, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    expectWriteNewFrames(m_maxFrame / 4, m_maxFrame / 4);
    expectCancelTimer();
    writeBufferSuccess(m_maxFrame / 4);

    // Check buffer now empty
    expectedWebAudioShmInfo->lengthMain = (m_maxFrame / 2) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetMain = (m_maxFrame / 4) * m_bytesPerFrame + m_dataOffset;
    expectedWebAudioShmInfo->lengthWrap = 0;
    expectedWebAudioShmInfo->offsetWrap = m_dataOffset;
    getBufferAvailableSuccess(m_maxFrame);
}

/**
 * Test that the stored data and new data is successfully written to gstreamer.
 * The new data stored in the shared memory has wrapped.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeAllStoredDataAndWrappedNewData)
{
    // Store data in the 3rd quater of the shared memory
    getBufferAvailableSuccess(m_maxFrame);
    expectWriteNewFrames(m_maxFrame * 3 / 4, m_maxFrame / 2);
    expectStartTimer();
    writeBufferSuccess(m_maxFrame * 3 / 4);

    // Write data that wraps the shared memory
    std::shared_ptr<WebAudioShmInfo> expectedWebAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    expectedWebAudioShmInfo->lengthMain = (m_maxFrame / 4) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetMain = (m_maxFrame * 3 / 4) * m_bytesPerFrame + m_dataOffset;
    expectedWebAudioShmInfo->lengthWrap = (m_maxFrame / 2) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetWrap = m_dataOffset;
    getBufferAvailableSuccess(m_maxFrame - m_framesStored, expectedWebAudioShmInfo);
    expectWriteStoredFrames(m_framesStored, m_framesStored);
    expectWriteNewFrames(m_maxFrame / 2, m_maxFrame / 2);
    expectCancelTimer();
    writeBufferSuccess(m_maxFrame / 2);

    // Check buffer now empty
    expectedWebAudioShmInfo->lengthMain = (m_maxFrame / 2) * m_bytesPerFrame;
    expectedWebAudioShmInfo->offsetMain = (m_maxFrame / 4) * m_bytesPerFrame;
    expectedWebAudioShmInfo->lengthWrap = 0;
    expectedWebAudioShmInfo->offsetWrap = 0;
    getBufferAvailableSuccess(m_maxFrame);
}

/**
 * Test that the stored data is written on the writeBufferTimer.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, handleWriteBufferTimerAllData)
{
    getBufferAvailableSuccess(m_maxFrame);
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
    getBufferAvailableSuccess(m_maxFrame);
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

/**
 * Test that getBufferDelay fails if gst player returns queued bytes larger than a uint32_t.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, getBufferDelayLargeQueuedBytes)
{
    uint32_t returnDelayFrames{};

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getQueuedBytes()).WillOnce(Return(4294967295 * m_bytesPerFrame + m_bytesPerFrame));

    bool status = m_webAudioPlayer->getBufferDelay(returnDelayFrames);
    EXPECT_EQ(status, false);
}

/**
 * Test that getBufferDelay returns the delayed frames if data queued in shm and gst.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, getBufferDelayQueuedBytes)
{
    // Fill the shared memory with data
    getBufferAvailableSuccess(m_maxFrame);
    expectWriteNewFrames(m_maxFrame / 2, 0);
    expectStartTimer();
    writeBufferSuccess(m_maxFrame / 2);

    uint32_t returnDelayFrames{};
    const uint64_t kFramesInGst = 400;
    const uint64_t kFramesInShm = m_maxFrame / 2;

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getQueuedBytes()).WillOnce(Return(kFramesInGst * m_bytesPerFrame));

    bool status = m_webAudioPlayer->getBufferDelay(returnDelayFrames);
    EXPECT_EQ(status, true);
    EXPECT_EQ(returnDelayFrames, kFramesInShm + kFramesInGst);

    expectCancelTimer();
}
