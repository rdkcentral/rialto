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
constexpr int sessionId{0};
constexpr firebolt::rialto::MediaSourceType validMediaSourceType{firebolt::rialto::MediaSourceType::VIDEO};
constexpr int sourceId = static_cast<int>(validMediaSourceType);
constexpr std::uint32_t bufferLen{7 * 1024 * 1024};
constexpr std::uint32_t metadataOffset{1024};
constexpr int requestId{0};
constexpr int maxFrames{24};
constexpr int maxMetadataBytes{2500};
} // namespace

namespace firebolt::rialto
{
bool operator==(const std::shared_ptr<ShmInfo> lhs, const std::shared_ptr<ShmInfo> rhs)
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

void NeedMediaDataTests::initialize()
{
    EXPECT_CALL(shmBufferMock, getMaxDataLen(sessionId, validMediaSourceType)).WillOnce(Return(bufferLen));
    EXPECT_CALL(shmBufferMock, getDataOffset(sessionId, validMediaSourceType)).WillOnce(Return(metadataOffset));
    m_sut = std::make_unique<firebolt::rialto::server::NeedMediaData>(m_clientMock, activeRequestsMock, shmBufferMock,
                                                                      sessionId, validMediaSourceType);
}

void NeedMediaDataTests::initializeWithWrongType()
{
    m_sut = std::make_unique<firebolt::rialto::server::NeedMediaData>(m_clientMock, activeRequestsMock, shmBufferMock,
                                                                      sessionId,
                                                                      firebolt::rialto::MediaSourceType::UNKNOWN);
}

void NeedMediaDataTests::needMediaDataWillBeSent()
{
    std::shared_ptr<firebolt::rialto::ShmInfo> expectedShmInfo{std::make_shared<firebolt::rialto::ShmInfo>()};
    expectedShmInfo->maxMetadataBytes = maxMetadataBytes;
    expectedShmInfo->metadataOffset = metadataOffset;
    expectedShmInfo->mediaDataOffset = metadataOffset + maxMetadataBytes;
    ASSERT_TRUE(m_sut);
    EXPECT_CALL(activeRequestsMock, insert(validMediaSourceType, _)).WillOnce(Return(requestId));
    EXPECT_CALL(*m_clientMock, notifyNeedMediaData(sourceId, maxFrames, requestId, expectedShmInfo));
    EXPECT_TRUE(m_sut->send());
}

void NeedMediaDataTests::needMediaDataWillNotBeSent()
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->send());
}
