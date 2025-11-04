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

#include "NeedMediaDataTestsFixture.h"

using testing::_;
using testing::Return;

namespace
{
constexpr int kSessionId{0};
constexpr firebolt::rialto::MediaSourceType kValidMediaSourceType{firebolt::rialto::MediaSourceType::VIDEO};
constexpr int kSourceId{1};
constexpr std::uint32_t kBufferLen{7 * 1024 * 1024};
constexpr std::uint32_t kMetadataOffset{1024};
constexpr int kRequestId{0};
constexpr int kMaxFrames{24};
constexpr int kMaxMetadataBytes{2500};
} // namespace

namespace firebolt::rialto
{
bool operator==(const std::shared_ptr<MediaPlayerShmInfo> lhs, const std::shared_ptr<MediaPlayerShmInfo> rhs)
{
    if (!lhs && !rhs)
        return true;
    if (!lhs)
        return false;
    if (!rhs)
        return false;
    return lhs->maxMetadataBytes == rhs->maxMetadataBytes && lhs->metadataOffset == rhs->metadataOffset &&
           lhs->mediaDataOffset == rhs->mediaDataOffset;
}
} // namespace firebolt::rialto

NeedMediaDataTests::NeedMediaDataTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::MediaPipelineClientMock>>()}
{
}

void NeedMediaDataTests::initialize(firebolt::rialto::PlaybackState playbackState)
{
    EXPECT_CALL(shmBufferMock, getMaxDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                             kSessionId, kValidMediaSourceType))
        .WillOnce(Return(kBufferLen));
    EXPECT_CALL(shmBufferMock, getDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                             kSessionId, kValidMediaSourceType))
        .WillOnce(Return(kMetadataOffset));
    m_sut = std::make_unique<firebolt::rialto::server::NeedMediaData>(m_clientMock, activeRequestsMock, shmBufferMock,
                                                                      kSessionId, kValidMediaSourceType, kSourceId,
                                                                      playbackState);
}

void NeedMediaDataTests::initializeWithWrongType()
{
    m_sut =
        std::make_unique<firebolt::rialto::server::NeedMediaData>(m_clientMock, activeRequestsMock, shmBufferMock,
                                                                  kSessionId, firebolt::rialto::MediaSourceType::UNKNOWN,
                                                                  kSourceId, firebolt::rialto::PlaybackState::PLAYING);
}

void NeedMediaDataTests::needMediaDataWillBeSentInPlayingState()
{
    std::shared_ptr<firebolt::rialto::MediaPlayerShmInfo> expectedShmInfo{
        std::make_shared<firebolt::rialto::MediaPlayerShmInfo>()};
    expectedShmInfo->maxMetadataBytes = kMaxMetadataBytes;
    expectedShmInfo->metadataOffset = kMetadataOffset;
    expectedShmInfo->mediaDataOffset = kMetadataOffset + kMaxMetadataBytes;
    ASSERT_TRUE(m_sut);
    EXPECT_CALL(activeRequestsMock, insert(kValidMediaSourceType, _)).WillOnce(Return(kRequestId));
    EXPECT_CALL(*m_clientMock, notifyNeedMediaData(kSourceId, kMaxFrames, kRequestId, expectedShmInfo));
    EXPECT_TRUE(m_sut->send());
}

void NeedMediaDataTests::needMediaDataWillNotBeSent()
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->send());
}
