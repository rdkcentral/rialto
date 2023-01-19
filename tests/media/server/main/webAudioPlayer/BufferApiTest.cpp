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
    std::shared_ptr<WebAudioShmInfo> m_webAudioShmInfo;

    RialtoServerWebAudioPlayerBufferApiTest() { createWebAudioPlayer(); }

    ~RialtoServerWebAudioPlayerBufferApiTest() { destroyWebAudioPlayer(); }

    void getBufferAvailableSuccess()
    {
        m_webAudioShmInfo = std::make_shared<WebAudioShmInfo>();
        mainThreadWillEnqueueTaskAndWait();
        bool status = m_webAudioPlayer->getBufferAvailable(m_availableFrames, m_webAudioShmInfo);
        EXPECT_EQ(status, true);
    }

    void writeBufferSuccess(uint32_t framesWritten)
    {
        mainThreadWillEnqueueTaskAndWait();
        EXPECT_CALL(*m_gstPlayerMock, writeBuffer(_, _, _, _)).WillOnce(Return(framesWritten * 4));
        bool status = m_webAudioPlayer->writeBuffer(framesWritten, nullptr);
        EXPECT_EQ(status, true);
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
 * Test that getBufferAvailable returns avaialble frames and shm information after construction of the
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
    getBufferAvailableSuccess();

    uint8_t *expectedMainPtr = &m_dataPtr + m_webAudioShmInfo->offsetMain;
    uint32_t expectedMainLength = m_dataLen;
    uint8_t *expectedWrapPtr = nullptr;
    uint32_t expectedWrapLength = 0;
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, writeBuffer(expectedMainPtr, expectedMainLength, expectedWrapPtr, expectedWrapLength))
        .WillOnce(Return(m_availableFrames * 4));
    bool status = m_webAudioPlayer->writeBuffer(m_availableFrames, nullptr);
    EXPECT_EQ(status, true);
}

/**
 * Test that writeBuffer fails if there was no getBufferAvailable call previous.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeBufferWithNoGetBufferAvailable)
{
    writeBufferFailure(123);
}

/**
 * Test that writeBuffer fails if there was no getBufferAvailable call previous.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, writeBufferBytesWrittenInvalid)
{
    getBufferAvailableSuccess();

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, writeBuffer(_, _, _, _)).WillOnce(Return(m_availableFrames));
    bool status = m_webAudioPlayer->writeBuffer(m_availableFrames, nullptr);
    EXPECT_EQ(status, false);
}

/**
 * Test that writeBuffer only succeeds once per getBufferAvailable call.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, multipleWriteBufferWithOneGetBufferAvailable)
{
    getBufferAvailableSuccess();

    writeBufferSuccess(m_availableFrames / 2);

    writeBufferFailure(m_availableFrames / 2);

    writeBufferFailure(m_availableFrames / 2);
}

/**
 * Test that subsequent getBufferAvailable calls overwrite the previous if a writeBuffer has not been called.
 */
TEST_F(RialtoServerWebAudioPlayerBufferApiTest, multipleGetBufferAvailable)
{
    getBufferAvailableSuccess();

    getBufferAvailableSuccess();

    getBufferAvailableSuccess();

    writeBufferSuccess(m_availableFrames / 2);

    writeBufferFailure(m_availableFrames / 2);
}
