/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include "TextTrackSession.h"
#include "TextTrackAccessorFactoryMock.h"
#include "TextTrackAccessorMock.h"
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>

using firebolt::rialto::server::ITextTrackAccessor;
using firebolt::rialto::server::TextTrackAccessorFactoryMock;
using firebolt::rialto::server::TextTrackAccessorMock;
using firebolt::rialto::server::TextTrackSession;
using testing::Return;
using testing::StrictMock;

namespace
{
constexpr std::uint32_t kSessionId{1};
constexpr bool kMute{true};
constexpr std::uint64_t kMediaTimestampMs{1234};
constexpr std::int32_t kDisplayOffsetMs{321};
const std::string kDisplayName{"DisplayName"};
const std::string kService{"service"};
const std::string kData{"DATA"};
} // namespace

class TextTrackSessionTest : public testing::Test
{
protected:
    void createSut()
    {
        EXPECT_CALL(m_accessorFactoryMock, getTextTrackAccessor()).WillOnce(Return(m_accessorMock));
        EXPECT_CALL(*m_accessorMock, openSession(kDisplayName)).WillOnce(Return(kSessionId));
        EXPECT_NO_THROW(m_sut = std::make_unique<TextTrackSession>(kDisplayName, m_accessorFactoryMock));

        // in teardown:
        EXPECT_CALL(*m_accessorMock, closeSession(kSessionId)).WillOnce(Return(true));
    }

    StrictMock<TextTrackAccessorFactoryMock> m_accessorFactoryMock;
    std::shared_ptr<TextTrackAccessorMock> m_accessorMock{std::make_shared<StrictMock<TextTrackAccessorMock>>()};
    std::unique_ptr<TextTrackSession> m_sut;
};

TEST_F(TextTrackSessionTest, ShouldFailToCreateWhenAccessorCantBeCreated)
{
    EXPECT_CALL(m_accessorFactoryMock, getTextTrackAccessor()).WillOnce(Return(nullptr));
    EXPECT_THROW(m_sut = std::make_unique<TextTrackSession>(kDisplayName, m_accessorFactoryMock), std::runtime_error);
}

TEST_F(TextTrackSessionTest, ShouldFailToCreateWhenSessionCantBeOpened)
{
    EXPECT_CALL(m_accessorFactoryMock, getTextTrackAccessor()).WillOnce(Return(m_accessorMock));
    EXPECT_CALL(*m_accessorMock, openSession(kDisplayName)).WillOnce(Return(std::nullopt));
    EXPECT_THROW(m_sut = std::make_unique<TextTrackSession>(kDisplayName, m_accessorFactoryMock), std::runtime_error);
}

TEST_F(TextTrackSessionTest, ShouldCreateTextTrackSession)
{
    createSut();
}

TEST_F(TextTrackSessionTest, shouldPause)
{
    createSut();
    EXPECT_CALL(*m_accessorMock, pause(kSessionId)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->pause());
}

TEST_F(TextTrackSessionTest, shouldPlay)
{
    createSut();
    EXPECT_CALL(*m_accessorMock, play(kSessionId)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->play());
}

TEST_F(TextTrackSessionTest, shouldMute)
{
    createSut();
    EXPECT_CALL(*m_accessorMock, mute(kSessionId, kMute)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->mute(kMute));
}

TEST_F(TextTrackSessionTest, shouldSetPosition)
{
    createSut();
    EXPECT_CALL(*m_accessorMock, setPosition(kSessionId, kMediaTimestampMs)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->setPosition(kMediaTimestampMs));
}

TEST_F(TextTrackSessionTest, shouldFailToSendCCData)
{
    createSut();
    EXPECT_CALL(*m_accessorMock, setSessionCCSelection(kSessionId, kService)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->setSessionCCSelection(kService));
    EXPECT_TRUE(m_sut->isClosedCaptions());

    EXPECT_CALL(*m_accessorMock, sendData(kSessionId, kData, ITextTrackAccessor::DataType::CC, kDisplayOffsetMs))
        .WillOnce(Return(false));
    EXPECT_FALSE(m_sut->sendData(kData, kDisplayOffsetMs));
}

TEST_F(TextTrackSessionTest, shouldSendWebVTTData)
{
    createSut();
    EXPECT_CALL(*m_accessorMock, setSessionWebVTTSelection(kSessionId)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->setSessionWebVTTSelection());
    EXPECT_FALSE(m_sut->isClosedCaptions());

    EXPECT_CALL(*m_accessorMock, sendData(kSessionId, kData, ITextTrackAccessor::DataType::WebVTT, kDisplayOffsetMs))
        .WillOnce(Return(true));
    EXPECT_TRUE(m_sut->sendData(kData, kDisplayOffsetMs));
}

TEST_F(TextTrackSessionTest, shouldSendTTMLData)
{
    createSut();
    EXPECT_CALL(*m_accessorMock, setSessionTTMLSelection(kSessionId)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->setSessionTTMLSelection());
    EXPECT_FALSE(m_sut->isClosedCaptions());

    EXPECT_CALL(*m_accessorMock, sendData(kSessionId, kData, ITextTrackAccessor::DataType::TTML, kDisplayOffsetMs))
        .WillOnce(Return(true));
    EXPECT_TRUE(m_sut->sendData(kData, kDisplayOffsetMs));
}

TEST_F(TextTrackSessionTest, shouldResetSession)
{
    createSut();
    EXPECT_CALL(*m_accessorMock, setSessionTTMLSelection(kSessionId)).Times(2).WillRepeatedly(Return(true));
    EXPECT_TRUE(m_sut->setSessionTTMLSelection());

    EXPECT_CALL(*m_accessorMock, resetSession(kSessionId)).WillOnce(Return(true));
    EXPECT_CALL(*m_accessorMock, mute(kSessionId, true)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->resetSession(true));
}

TEST_F(TextTrackSessionTest, shouldAssociateVideoDecoder)
{
    uint8_t decoder{0};
    uintptr_t decoderAddress = reinterpret_cast<uintptr_t>(&decoder);
    std::string decoderAddressStr = std::to_string(decoderAddress);
    createSut();
    EXPECT_CALL(*m_accessorMock, associateVideoDecoder(kSessionId, decoderAddressStr)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->associateVideoDecoder(&decoder));
}

TEST_F(TextTrackSessionTest, shouldFailToAssociateVideoDecoder)
{
    uint8_t decoder{0};
    uintptr_t decoderAddress = reinterpret_cast<uintptr_t>(&decoder);
    std::string decoderAddressStr = std::to_string(decoderAddress);
    createSut();
    EXPECT_CALL(*m_accessorMock, associateVideoDecoder(kSessionId, decoderAddressStr)).WillOnce(Return(false));
    EXPECT_FALSE(m_sut->associateVideoDecoder(&decoder));
}
