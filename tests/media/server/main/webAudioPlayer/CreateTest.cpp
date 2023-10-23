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

using ::testing::Throw;

class RialtoServerCreateWebAudioPlayerTest : public WebAudioPlayerTestBase
{
protected:
};

/**
 * Test that a WebAudioPlayer object can be created successfully.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, Create)
{
    mainThreadWillEnqueueTaskAndWait();
    expectConstructionOfWebAudioPlayerServerInternal();

    EXPECT_NO_THROW(m_webAudioPlayer =
                        std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                       m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                       m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                       m_gstPlayerFactoryMock, m_timerFactoryMock));
    EXPECT_NE(m_webAudioPlayer, nullptr);

    destroyWebAudioPlayer();
}

/**
 * Test the external factory (this code is designed to fail)
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, ExternalFactoryFailure)
{
    std::shared_ptr<firebolt::rialto::IWebAudioPlayerFactory> factory =
        firebolt::rialto::IWebAudioPlayerFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    // The following is expected to return null, and show an error log
    EXPECT_EQ(factory->createWebAudioPlayer(m_webAudioPlayerClientMock, m_audioMimeType, m_priority, &m_config), nullptr);
}

/**
 * Test the internal factory
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, InternalFactoryCreatesObject)
{
    std::shared_ptr<IWebAudioPlayerServerInternalFactory> factory =
        firebolt::rialto::server::IWebAudioPlayerServerInternalFactory::createFactory();
    EXPECT_NE(factory, nullptr);

    mainThreadWillEnqueueTaskAndWait();
    expectConstructionOfWebAudioPlayerServerInternal();

    std::unique_ptr<IWebAudioPlayer> webAudioPlayerServer;
    EXPECT_NO_THROW(webAudioPlayerServer =
                        factory->createWebAudioPlayerServerInternal(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock));
    EXPECT_NE(webAudioPlayerServer, nullptr);

    // The returned object (IWebAudioPlayer) is expected to be a WebAudioPlayerServerInternal
    m_webAudioPlayer.reset(dynamic_cast<WebAudioPlayerServerInternal *>(webAudioPlayerServer.release()));
    EXPECT_NE(m_webAudioPlayer, nullptr);
    destroyWebAudioPlayer();
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
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer object throws an exeception if the bytes per frame is 0.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, InvalidBytesPerFrame)
{
    m_config.pcm.channels = 0;
    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock),
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
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock),
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
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock),
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
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer object throws an exeception if faiure to get the shared buffer ptr.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, GetBufferFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer object throws an exeception if faiure to get the offset of the web audio partition.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, GetDataOffsetFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&m_dataPtr));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Throw(std::runtime_error("Fail")));

    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer object throws an exeception if faiure to get the maximum data length for the web audio partition.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, GetMaxDataLenFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                mapPartition(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, m_webAudioPlayerHandle))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&m_dataPtr));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(m_dataOffset));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(0));

    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock),
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
    EXPECT_CALL(*m_sharedMemoryBufferMock, getBuffer()).WillOnce(Return(&m_dataPtr));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getDataOffset(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(m_dataOffset));
    EXPECT_CALL(*m_sharedMemoryBufferMock, getMaxDataLen(ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                                         m_webAudioPlayerHandle, MediaSourceType::AUDIO))
        .WillOnce(Return(m_dataLen));
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstWebAudioPlayer(_, m_priority)).WillOnce(Return(nullptr));
    EXPECT_THROW(m_webAudioPlayer =
                     std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                    m_priority, &m_config, m_sharedMemoryBufferMock,
                                                                    m_webAudioPlayerHandle, m_mainThreadFactoryMock,
                                                                    m_gstPlayerFactoryMock, m_timerFactoryMock),
                 std::runtime_error);
}
