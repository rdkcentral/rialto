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
#include "RialtoLogging.h"
#include "TextTrackPluginWrapperMock.h"
#include "TextTrackWrapperMock.h"
#include "ThunderWrapperMock.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

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
constexpr char kSubtitleDataEnv[] = "RIALTO_SHOW_SUBTITLE_DATA";
constexpr char kSubtitleLogPathEnv[] = "RIALTO_SUBTITLE_LOG_PATH";
std::vector<std::string> g_logMessages;

void captureLog(RIALTO_DEBUG_LEVEL, const char *, int, const char *, const char *message, std::size_t messageLen)
{
    g_logMessages.emplace_back(message, messageLen);
}

#ifdef RIALTO_LOG_DEBUG_ENABLED
std::string getLoggedSubtitlePayload()
{
    std::string loggedPayload;
    for (const auto &message : g_logMessages)
    {
        if (message.rfind("Subtitle data", 0) != 0)
        {
            continue;
        }

        const std::size_t separator = message.find(": ");
        if (separator != std::string::npos)
        {
            loggedPayload += message.substr(separator + 2);
        }
    }
    return loggedPayload;
}
#endif

void expectLoggedSubtitlePayload(const std::string &payload)
{
#ifdef RIALTO_LOG_DEBUG_ENABLED
    EXPECT_EQ(getLoggedSubtitlePayload(), payload);
#else
    static_cast<void>(payload);
    EXPECT_THAT(g_logMessages, testing::Each(testing::Not(testing::HasSubstr("Subtitle data:"))));
#endif
}
} // namespace

class TextTrackAccessorTests : public testing::Test
{
protected:
    void SetUp() override
    {
        if (const char *value = std::getenv(kSubtitleDataEnv))
        {
            m_originalSubtitleDataEnv = value;
        }
        if (const char *value = std::getenv(kSubtitleLogPathEnv))
        {
            m_originalSubtitleLogPathEnv = value;
        }
        unsetenv(kSubtitleDataEnv);
        unsetenv(kSubtitleLogPathEnv);
        g_logMessages.clear();
        ASSERT_EQ(firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_SERVER, captureLog, true),
                  firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK);
    }

    void TearDown() override
    {
        EXPECT_EQ(firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_SERVER, nullptr, false),
                  firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK);
        if (m_originalSubtitleDataEnv)
        {
            setenv(kSubtitleDataEnv, m_originalSubtitleDataEnv->c_str(), 1);
        }
        else
        {
            unsetenv(kSubtitleDataEnv);
        }
        if (m_originalSubtitleLogPathEnv)
        {
            setenv(kSubtitleLogPathEnv, m_originalSubtitleLogPathEnv->c_str(), 1);
        }
        else
        {
            unsetenv(kSubtitleLogPathEnv);
        }
        if (!m_subtitleLogPath.empty())
        {
            unlink(m_subtitleLogPath.c_str());
            rmdir(m_subtitleLogPath.c_str());
        }
        if (!m_subtitleLogTargetPath.empty())
        {
            std::remove(m_subtitleLogTargetPath.c_str());
        }
    }

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
    std::optional<std::string> m_originalSubtitleDataEnv;
    std::optional<std::string> m_originalSubtitleLogPathEnv;
    std::string m_subtitleLogPath;
    std::string m_subtitleLogTargetPath;
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

TEST_F(TextTrackAccessorTests, ShouldLogSubtitleDataWhenEnvironmentVariableIsEnabled)
{
    setenv(kSubtitleDataEnv, "1", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    expectLoggedSubtitlePayload(kData);
}

TEST_F(TextTrackAccessorTests, ShouldTreatSubtitleDataEnvironmentVariableCaseInsensitively)
{
    setenv(kSubtitleDataEnv, "TrUe", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    expectLoggedSubtitlePayload(kData);
}

TEST_F(TextTrackAccessorTests, ShouldNotLogSubtitleDataWhenEnvironmentVariableIsUnset)
{
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Each(testing::Not(testing::HasSubstr("Subtitle data:"))));
}

TEST_F(TextTrackAccessorTests, ShouldLogEmptySubtitleData)
{
    setenv(kSubtitleDataEnv, "yes", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, std::string{}))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, {}, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
#ifdef RIALTO_LOG_DEBUG_ENABLED
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("Subtitle data:")));
#endif
    expectLoggedSubtitlePayload("");
}

TEST_F(TextTrackAccessorTests, ShouldNotLogSubtitleDataWhenDebugLevelIsDisabled)
{
    ASSERT_EQ(firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_SERVER, captureLog, false),
              firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK);
    const RIALTO_DEBUG_LEVEL originalLogLevels{firebolt::rialto::logging::getLogLevels(RIALTO_COMPONENT_SERVER)};
    ASSERT_EQ(firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_SERVER, RIALTO_DEBUG_LEVEL_MILESTONE),
              firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK);
    setenv(kSubtitleDataEnv, "1", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Each(testing::Not(testing::HasSubstr("Subtitle data:"))));

    EXPECT_EQ(firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_SERVER, originalLogLevels),
              firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK);
    EXPECT_EQ(firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_SERVER, captureLog, true),
              firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK);
}

TEST_F(TextTrackAccessorTests, ShouldLogSubtitleDataNormallyWhenFileLoggingIsDisabled)
{
    setenv(kSubtitleDataEnv, "1", 1);
    setenv(kSubtitleLogPathEnv, "", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    expectLoggedSubtitlePayload(kData);
}

TEST_F(TextTrackAccessorTests, ShouldRejectPathTraversalAndFallBackToDebug)
{
    const std::string largeData(1800, 'x');
    setenv(kSubtitleLogPathEnv, "/tmp/../etc/rialto-subtitles.log", 1);
    setenv(kSubtitleDataEnv, "1", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, largeData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, largeData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("Failed to capture subtitle data")));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("directory must be exactly /opt/logs or /tmp")));
    expectLoggedSubtitlePayload(largeData);
}

TEST_F(TextTrackAccessorTests, ShouldReportInvalidSubtitleLogPaths)
{
    setenv(kSubtitleLogPathEnv, "relative.log", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .Times(2)
        .WillRepeatedly(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).Times(2).WillRepeatedly(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    setenv(kSubtitleLogPathEnv, "/tmp/invalid name.log", 1);
    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));

    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("path must be absolute")));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("filename must contain only letters")));
}

TEST_F(TextTrackAccessorTests, ShouldReportSubtitleLogPathOpenFailures)
{
    m_subtitleLogPath = "/tmp/rialto-subtitle-directory-" + std::to_string(getpid()) + ".log";
    rmdir(m_subtitleLogPath.c_str());
    ASSERT_EQ(mkdir(m_subtitleLogPath.c_str(), 0700), 0);
    setenv(kSubtitleLogPathEnv, m_subtitleLogPath.c_str(), 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("cannot open log file:")));
}

TEST_F(TextTrackAccessorTests, ShouldReportSubtitleLogDirectoryOpenFailures)
{
    struct stat directoryStatus
    {
    };
    if (stat("/opt/logs", &directoryStatus) == 0)
    {
        GTEST_SKIP() << "/opt/logs exists, so its open failure cannot be tested safely";
    }

    m_subtitleLogPath = "/opt/logs/rialto-subtitle-" + std::to_string(getpid()) + ".log";
    setenv(kSubtitleLogPathEnv, m_subtitleLogPath.c_str(), 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("cannot open log directory:")));
}

TEST_F(TextTrackAccessorTests, ShouldNotFollowSymlinkAtSubtitleLogPath)
{
    const std::string fileSuffix{"-" + std::to_string(getpid()) + ".log"};
    m_subtitleLogPath = "/tmp/rialto-subtitle-link" + fileSuffix;
    m_subtitleLogTargetPath = "/tmp/rialto-subtitle-target" + fileSuffix;
    std::remove(m_subtitleLogPath.c_str());
    std::remove(m_subtitleLogTargetPath.c_str());
    {
        std::ofstream targetFile{m_subtitleLogTargetPath};
        ASSERT_TRUE(targetFile.is_open());
        targetFile << "original content\n";
    }
    ASSERT_EQ(symlink(m_subtitleLogTargetPath.c_str(), m_subtitleLogPath.c_str()), 0);
    setenv(kSubtitleLogPathEnv, m_subtitleLogPath.c_str(), 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("Failed to capture subtitle data")));
    expectLoggedSubtitlePayload("");
    std::ifstream targetFile{m_subtitleLogTargetPath};
    ASSERT_TRUE(targetFile.is_open());
    const std::string targetContents{std::istreambuf_iterator<char>{targetFile}, std::istreambuf_iterator<char>{}};
    EXPECT_EQ(targetContents, "original content\n");
}

TEST_F(TextTrackAccessorTests, ShouldRejectHardLinkedSubtitleLogFile)
{
    const std::string fileSuffix{"-" + std::to_string(getpid()) + ".log"};
    m_subtitleLogPath = "/tmp/rialto-subtitle-hardlink" + fileSuffix;
    m_subtitleLogTargetPath = "/tmp/rialto-subtitle-hardlink-target" + fileSuffix;
    std::remove(m_subtitleLogPath.c_str());
    std::remove(m_subtitleLogTargetPath.c_str());
    {
        std::ofstream targetFile{m_subtitleLogTargetPath};
        ASSERT_TRUE(targetFile.is_open());
        targetFile << "original content\n";
    }
    ASSERT_EQ(link(m_subtitleLogTargetPath.c_str(), m_subtitleLogPath.c_str()), 0);
    setenv(kSubtitleLogPathEnv, m_subtitleLogPath.c_str(), 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("Failed to capture subtitle data")));
    std::ifstream targetFile{m_subtitleLogTargetPath};
    ASSERT_TRUE(targetFile.is_open());
    const std::string targetContents{std::istreambuf_iterator<char>{targetFile}, std::istreambuf_iterator<char>{}};
    EXPECT_EQ(targetContents, "original content\n");
}

TEST_F(TextTrackAccessorTests, ShouldRejectSubtitleLogFileOwnedByAnotherUser)
{
    const std::string fileSuffix{"-" + std::to_string(getpid()) + ".log"};
    m_subtitleLogPath = "/tmp/rialto-subtitle-owner" + fileSuffix;
    std::remove(m_subtitleLogPath.c_str());
    {
        std::ofstream logFile{m_subtitleLogPath};
        ASSERT_TRUE(logFile.is_open());
        logFile << "original content\n";
    }

    struct stat originalFileStatus
    {
    };
    ASSERT_EQ(stat(m_subtitleLogPath.c_str(), &originalFileStatus), 0);
    const uid_t otherUid = (geteuid() == 0) ? 65534 : geteuid() + 1;
    if (chown(m_subtitleLogPath.c_str(), otherUid, originalFileStatus.st_gid) != 0)
    {
        std::remove(m_subtitleLogPath.c_str());
        m_subtitleLogPath.clear();
        GTEST_SKIP() << "Changing temporary file ownership is unavailable";
    }
    if (chown(m_subtitleLogPath.c_str(), otherUid, getegid()) != 0 || chmod(m_subtitleLogPath.c_str(), 0660) != 0)
    {
        chown(m_subtitleLogPath.c_str(), originalFileStatus.st_uid, originalFileStatus.st_gid);
        std::remove(m_subtitleLogPath.c_str());
        m_subtitleLogPath.clear();
        GTEST_SKIP() << "Making the temporary file writable is unavailable";
    }

    setenv(kSubtitleLogPathEnv, m_subtitleLogPath.c_str(), 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("Failed to capture subtitle data")));
    ASSERT_EQ(chown(m_subtitleLogPath.c_str(), originalFileStatus.st_uid, originalFileStatus.st_gid), 0);
    std::ifstream logFile{m_subtitleLogPath};
    ASSERT_TRUE(logFile.is_open());
    const std::string logContents{std::istreambuf_iterator<char>{logFile}, std::istreambuf_iterator<char>{}};
    EXPECT_EQ(logContents, "original content\n");
}

TEST_F(TextTrackAccessorTests, ShouldReturnWhenFifoHasNoReader)
{
    const std::string fifoPath = "/tmp/rialto-subtitle-fifo-" + std::to_string(getpid());
    unlink(fifoPath.c_str());
    ASSERT_EQ(mkfifo(fifoPath.c_str(), 0600), 0);
    setenv(kSubtitleLogPathEnv, fifoPath.c_str(), 1);
    createSut();

    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("Failed to capture subtitle data")));
    EXPECT_THAT(g_logMessages, testing::Contains(testing::HasSubstr("cannot open log file:")));
    unlink(fifoPath.c_str());
}

TEST_F(TextTrackAccessorTests, ShouldNotLogSubtitleDataWhenEnvironmentVariableIsFalse)
{
    setenv(kSubtitleDataEnv, "false", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, kData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_THAT(g_logMessages, testing::Each(testing::Not(testing::HasSubstr("Subtitle data:"))));
}

TEST_F(TextTrackAccessorTests, ShouldLogLargeSubtitleDataWithoutTruncation)
{
    std::string largeData(399, 'x');
    largeData += '\n';
    largeData += std::string(1400, 'y');
    setenv(kSubtitleDataEnv, "1", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, largeData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, largeData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    expectLoggedSubtitlePayload(largeData);
}

TEST_F(TextTrackAccessorTests, ShouldHandleSubtitleDataAtUtf8ChunkBoundaries)
{
    std::string utf8Data(399, 'x');
    utf8Data.push_back(static_cast<char>(0xc3));
    utf8Data.push_back(static_cast<char>(0xa9));
    utf8Data += std::string(500, 'y');
    const std::string invalidUtf8Data(401, static_cast<char>(0x80));
    setenv(kSubtitleDataEnv, "1", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, _))
        .Times(2)
        .WillRepeatedly(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).Times(2).WillRepeatedly(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, utf8Data, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    expectLoggedSubtitlePayload(utf8Data);
    g_logMessages.clear();
    EXPECT_TRUE(m_sut->sendData(kSessionId, invalidUtf8Data, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    expectLoggedSubtitlePayload(invalidUtf8Data);
}

TEST_F(TextTrackAccessorTests, ShouldEscapeEmbeddedNullBytesInDebugLogs)
{
    const std::string binaryData{"before\0after", 12};
    setenv(kSubtitleDataEnv, "1", 1);
    createSut();
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, binaryData))
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).WillOnce(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, binaryData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    expectLoggedSubtitlePayload("before\\0after");
}

TEST_F(TextTrackAccessorTests, ShouldAppendFileOutputAndPreferItOverDebugLogging)
{
    m_subtitleLogPath = "/tmp/rialto-subtitle-data-" + std::to_string(getpid()) + ".log";
    std::remove(m_subtitleLogPath.c_str());
    setenv(kSubtitleLogPathEnv, m_subtitleLogPath.c_str(), 1);
    setenv(kSubtitleDataEnv, "1", 1);
    createSut();

    const std::string firstData{"first cue"};
    const std::string secondData{"second cue"};
    EXPECT_CALL(*m_textTrackWrapperMock,
                sendSessionData(kSessionId, ITextTrackWrapper::DataType::TTML, kDisplayOffsetMs, _))
        .Times(2)
        .WillRepeatedly(Return(kNoError));
    EXPECT_CALL(*m_thunderWrapperMock, isSuccessful(kNoError)).Times(2).WillRepeatedly(Return(true));

    EXPECT_TRUE(m_sut->sendData(kSessionId, firstData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));
    EXPECT_TRUE(m_sut->sendData(kSessionId, secondData, TextTrackAccessor::DataType::TTML, kDisplayOffsetMs));

    std::ifstream subtitleLogFile{m_subtitleLogPath};
    ASSERT_TRUE(subtitleLogFile.is_open());
    const std::string loggedData{std::istreambuf_iterator<char>{subtitleLogFile}, std::istreambuf_iterator<char>{}};
    EXPECT_EQ(loggedData, "Subtitle data: first cue\nSubtitle data: second cue\n");
    EXPECT_THAT(g_logMessages, testing::Each(testing::Not(testing::HasSubstr("Subtitle data:"))));
}

TEST_F(TextTrackAccessorTests, ShouldSendDataForCCType)
{
    createSut();
    EXPECT_FALSE(m_sut->sendData(kSessionId, kData, TextTrackAccessor::DataType::CC, kDisplayOffsetMs));
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
