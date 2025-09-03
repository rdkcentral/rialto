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

#include "TextTrackAccessor.h"
#include "TextTrackPluginWrapperMock.h"
#include "TextTrackWrapperMock.h"
#include "ThunderWrapperMock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using firebolt::rialto::server::TextTrackAccessor;
using firebolt::rialto::wrappers::ITextTrackWrapper;
using firebolt::rialto::wrappers::TextTrackPluginWrapperMock;
using firebolt::rialto::wrappers::TextTrackWrapperMock;
using firebolt::rialto::wrappers::ThunderWrapperMock;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgReferee;
using testing::StrictMock;

namespace
{
constexpr std::uint32_t kNoError{0};
constexpr std::uint32_t kError{1};
constexpr std::uint32_t kSessionId{0};
constexpr std::uint64_t kPosition{1234};
constexpr std::int32_t kDisplayOffsetMs{4345};
const char *kErrorString{"ERROR"};
const std::string kDisplayName{"DisplayName"};
const std::string kData("data");
const std::string kService{"Service"};
const std::string kVideoDecoder{"1234"};
} // namespace

class TextTrackAccessorTests : public testing::Test
{
protected:
    void createSut()
    {
        EXPECT_CALL(*m_textTrackPluginWrapperMock, open()).WillOnce(Return(kNoError));
        EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
        EXPECT_CALL(*m_textTrackPluginWrapperMock, isOperational()).WillOnce(Return(true));
        EXPECT_CALL(*m_textTrackPluginWrapperMock, interface()).WillOnce(Return(m_textTrackWrapperMock));
        EXPECT_NO_THROW(m_sut = std::make_unique<TextTrackAccessor>(m_textTrackPluginWrapperMock, m_thunderWrapperMock));
    }

    std::shared_ptr<TextTrackWrapperMock> m_textTrackWrapperMock{std::make_shared<StrictMock<TextTrackWrapperMock>>()};
    std::shared_ptr<TextTrackPluginWrapperMock> m_textTrackPluginWrapperMock{
        std::make_shared<StrictMock<TextTrackPluginWrapperMock>>()};
    std::shared_ptr<ThunderWrapperMock> m_thunderWrapperMock{std::make_shared<StrictMock<ThunderWrapperMock>>()};
    std::unique_ptr<TextTrackAccessor> m_sut;
};

TEST_F(TextTrackAccessorTests, ShouldFailToCreateWhenWrapperIsNull)
{
    EXPECT_THROW(m_sut = std::make_unique<TextTrackAccessor>(nullptr, nullptr), std::exception);
}

TEST_F(TextTrackAccessorTests, ShouldFailToCreateWhenFailedToOpen)
{
    EXPECT_CALL(*m_textTrackPluginWrapperMock, open()).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_THROW(m_sut = std::make_unique<TextTrackAccessor>(m_textTrackPluginWrapperMock, m_thunderWrapperMock),
                 std::exception);
}

TEST_F(TextTrackAccessorTests, ShouldFailToCreateWhenNotOperational)
{
    EXPECT_CALL(*m_textTrackPluginWrapperMock, open()).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_CALL(*m_textTrackPluginWrapperMock, isOperational()).WillOnce(Return(false));
    EXPECT_THROW(m_sut = std::make_unique<TextTrackAccessor>(m_textTrackPluginWrapperMock, m_thunderWrapperMock),
                 std::exception);
}

TEST_F(TextTrackAccessorTests, ShouldFailToCreateWhenTextTrackWrapperIsNull)
{
    EXPECT_CALL(*m_textTrackPluginWrapperMock, open()).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_CALL(*m_textTrackPluginWrapperMock, isOperational()).WillOnce(Return(true));
    EXPECT_CALL(*m_textTrackPluginWrapperMock, interface()).WillOnce(Return(nullptr));
    EXPECT_THROW(m_sut = std::make_unique<TextTrackAccessor>(m_textTrackPluginWrapperMock, m_thunderWrapperMock),
                 std::exception);
}

TEST_F(TextTrackAccessorTests, ShouldCreate)
{
    createSut();
}

TEST_F(TextTrackAccessorTests, ShouldOpenSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, openSession(kDisplayName, _))
        .WillOnce(DoAll(SetArgReferee<1>(kSessionId), Return(kNoError)));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_EQ(m_sut->openSession(kDisplayName), kSessionId);
}

TEST_F(TextTrackAccessorTests, ShouldFailToOpenSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, openSession(kDisplayName, _)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_EQ(m_sut->openSession(kDisplayName), std::nullopt);
}

TEST_F(TextTrackAccessorTests, ShouldCloseSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, closeSession(kSessionId)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->closeSession(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldFailToCloseSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, closeSession(kSessionId)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->closeSession(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldPauseSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, pauseSession(kSessionId)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->pause(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldFailToPauseSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, pauseSession(kSessionId)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->pause(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldResumeSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, resumeSession(kSessionId)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->play(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldFailToResumeSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, resumeSession(kSessionId)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->play(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldMuteSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, muteSession(kSessionId)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->mute(kSessionId, true));
}

TEST_F(TextTrackAccessorTests, ShouldFailToMuteSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, muteSession(kSessionId)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->mute(kSessionId, true));
}

TEST_F(TextTrackAccessorTests, ShouldUnmuteSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, unmuteSession(kSessionId)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->mute(kSessionId, false));
}

TEST_F(TextTrackAccessorTests, ShouldFailToUnmuteSession)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, unmuteSession(kSessionId)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->mute(kSessionId, false));
}

TEST_F(TextTrackAccessorTests, ShouldSetPosition)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, sendSessionTimestamp(kSessionId, kPosition)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->setPosition(kSessionId, kPosition));
}

TEST_F(TextTrackAccessorTests, ShouldFailToSetPosition)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, sendSessionTimestamp(kSessionId, kPosition)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->setPosition(kSessionId, kPosition));
}

TEST_F(TextTrackAccessorTests, ShouldFailToSendDataForUnknownType)
{
    createSut();
    EXPECT_FALSE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::UNKNOWN, kDisplayOffsetMs));
}

TEST_F(TextTrackAccessorTests, ShouldSendDataForTtmlType)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
}

TEST_F(TextTrackAccessorTests, ShouldSendDataForWebVttType)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::WEBVTT, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::WebVTT, kDisplayOffsetMs));
}

TEST_F(TextTrackAccessorTests, ShouldFailToSendData)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
}

TEST_F(TextTrackAccessorTests, ShouldSetSessionWebVTTSelection)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, setSessionWebVTTSelection(kSessionId)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->setSessionWebVTTSelection(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldFailToSetSessionWebVTTSelection)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, setSessionWebVTTSelection(kSessionId)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->setSessionWebVTTSelection(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldSetSessionTTMLSelection)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, setSessionTTMLSelection(kSessionId)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->setSessionTTMLSelection(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldFailToSetSessionTTMLSelection)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, setSessionTTMLSelection(kSessionId)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->setSessionTTMLSelection(kSessionId));
}

TEST_F(TextTrackAccessorTests, ShouldSetSessionCCSelection)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, setSessionClosedCaptionsService(kSessionId, kService)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->setSessionCCSelection(kSessionId, kService));
}

TEST_F(TextTrackAccessorTests, ShouldFailToSetSessionCCSelection)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, setSessionClosedCaptionsService(kSessionId, kService)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->setSessionCCSelection(kSessionId, kService));
}

TEST_F(TextTrackAccessorTests, ShouldAssociateVideoDecoder)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, associateVideoDecoder(kSessionId, kVideoDecoder)).WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->associateVideoDecoder(kSessionId, kVideoDecoder));
}

TEST_F(TextTrackAccessorTests, ShouldFailToAssociateVideoDecoder)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock, associateVideoDecoder(kSessionId, kVideoDecoder)).WillOnce(Return(kError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kError)).WillOnce(Return(false));
    EXPECT_CALL(*m_thunderWrapperMock, errorToString(kError)).WillOnce(Return(kErrorString));
    EXPECT_FALSE(m_sut->associateVideoDecoder(kSessionId, kVideoDecoder));
}
