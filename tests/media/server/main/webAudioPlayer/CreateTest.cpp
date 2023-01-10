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

#include "WebAudioPlayerTestBase.h"

class RialtoServerCreateWebAudioPlayerTest : public WebAudioPlayerTestBase
{
};

/**
 * Test that a WebAudioPlayer object can be created successfully.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, Create)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstWebAudioPlayer(_)).WillOnce(Return(ByMove(std::move(m_gstPlayer))));

    EXPECT_NO_THROW(m_webAudioPlayer =
                        std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                       m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                       m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                       m_gstPlayerFactoryMock));
    EXPECT_NE(m_webAudioPlayer, nullptr);
}

/**
 * Test that a WebAudioPlayer object throws an exeception if the pcm config is null.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, InvalidPcmConfig)
{
    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, nullptr, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer object throws an exeception if the mime type is not supported.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, InvalidMimeType)
{
    const std::string invalidMimeType{"invalid"};

    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, invalidMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer object throws an exeception if the main thread could not be got.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, MainThreadFailure)
{
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(nullptr));
    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer object throws an exeception if the partition could not be mapped.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, MapPartitionFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
        .WillOnce(Return(false));
    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer object throws an exeception if faiure to create the gst player.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, GstPlayerFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstWebAudioPlayer(_)).WillOnce(Return(nullptr));
    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock),
                 std::runtime_error);
}
