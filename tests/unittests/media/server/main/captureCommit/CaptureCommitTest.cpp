/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "CaptureCommit.h"
#include "CaptureAdmissionMock.h"
#include "CaptureHalLibraryFactoryMock.h"
#include "CaptureHalLibraryMock.h"
#include "CaptureSessionMock.h"
#include "GstGenericPlayerMock.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using firebolt::rialto::wrappers::CaptureHalLibraryFactoryMock;
using firebolt::rialto::wrappers::CaptureHalLibraryMock;
using firebolt::rialto::wrappers::CaptureSessionPtr;
using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{
constexpr int kSessionId{3};
constexpr std::uint32_t kCaptureId{7};
} // namespace

class CaptureCommitTest : public ::testing::Test
{
protected:
    void TearDown() override { unsetenv("RIALTO_PLAYBIN_VIDEO_SINK"); }

    // The HAL creates one session; the events list records when it is deleted.
    void halWillCreateSession()
    {
        EXPECT_CALL(*m_halFactory, getCaptureHalLibrary()).WillOnce(Return(m_hal));
        EXPECT_CALL(*m_hal, createSession())
            .WillOnce(Invoke(
                [this]()
                {
                    m_session = new NiceMock<capture::CaptureSessionMock>();
                    return CaptureSessionPtr{m_session, [this](capture::ICaptureSession *session)
                                             {
                                                 m_events.push_back("session deleted");
                                                 delete session;
                                             }};
                }));
    }

    bool run() { return m_sut.run(m_player, kSessionId, kCaptureId, m_videoSource, m_committed); }

    std::shared_ptr<StrictMock<CaptureHalLibraryFactoryMock>> m_halFactory{
        std::make_shared<StrictMock<CaptureHalLibraryFactoryMock>>()};
    std::shared_ptr<StrictMock<CaptureHalLibraryMock>> m_hal{std::make_shared<StrictMock<CaptureHalLibraryMock>>()};
    std::shared_ptr<StrictMock<CaptureAdmissionMock>> m_admission{std::make_shared<StrictMock<CaptureAdmissionMock>>()};
    StrictMock<GstGenericPlayerMock> m_player;
    CaptureCommit m_sut{m_halFactory, m_admission};
    std::unique_ptr<IMediaPipeline::MediaSource> m_videoSource{
        std::make_unique<IMediaPipeline::MediaSourceVideo>("video/h264", false)};
    CaptureSessionHandle m_committed;
    capture::ICaptureSession *m_session{nullptr};
    std::vector<std::string> m_events;
};

TEST_F(CaptureCommitTest, EncryptedVideoFailsBeforeAnySession)
{
    m_videoSource = std::make_unique<IMediaPipeline::MediaSourceVideo>("video/h264", true);
    EXPECT_FALSE(run()); // no HAL call: the strict mocks would flag it
}

TEST_F(CaptureCommitTest, NoVideoSourceFails)
{
    m_videoSource.reset();
    EXPECT_FALSE(run());
}

TEST_F(CaptureCommitTest, OtherSinkOverrideFails)
{
    setenv("RIALTO_PLAYBIN_VIDEO_SINK", "autovideosink", 1);
    EXPECT_FALSE(run());
}

TEST_F(CaptureCommitTest, WesterossinkOverrideIsAccepted)
{
    setenv("RIALTO_PLAYBIN_VIDEO_SINK", "westerossink", 1);
    halWillCreateSession();
    EXPECT_CALL(m_player, configureCaptureSink(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*m_admission, admit(_)).WillOnce(Return(true));
    EXPECT_TRUE(run());
}

TEST_F(CaptureCommitTest, NoHalSessionFails)
{
    EXPECT_CALL(*m_halFactory, getCaptureHalLibrary()).WillOnce(Return(m_hal));
    EXPECT_CALL(*m_hal, createSession()).WillOnce(Return(ByMove(CaptureSessionPtr{})));
    EXPECT_FALSE(run());
}

TEST_F(CaptureCommitTest, SinkFailureDeletesTheSession)
{
    halWillCreateSession();
    EXPECT_CALL(m_player, configureCaptureSink(_, _))
        .WillOnce(Invoke(
            [this](void *session, const std::unique_ptr<IMediaPipeline::MediaSource> &source)
            {
                EXPECT_EQ(session, m_session);
                EXPECT_EQ(source.get(), m_videoSource.get());
                return false;
            }));
    EXPECT_FALSE(run());
    EXPECT_EQ(m_events, std::vector<std::string>{"session deleted"});
    EXPECT_EQ(m_committed.session, nullptr);
}

TEST_F(CaptureCommitTest, AdmissionFailureReleasesTheSinkBeforeDeletingTheSession)
{
    halWillCreateSession();
    EXPECT_CALL(m_player, configureCaptureSink(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*m_admission, admit(_))
        .WillOnce(Invoke(
            [](CaptureSessionHandle &handle)
            {
                EXPECT_EQ(handle.sessionId, kSessionId);
                EXPECT_EQ(handle.captureId, kCaptureId);
                return false;
            }));
    EXPECT_CALL(m_player, releaseCaptureSink()).WillOnce(Invoke([this]() { m_events.push_back("sink released"); }));
    EXPECT_FALSE(run());
    EXPECT_EQ(m_events, (std::vector<std::string>{"sink released", "session deleted"}));
    EXPECT_EQ(m_committed.session, nullptr);
}

TEST_F(CaptureCommitTest, SuccessHandsTheSessionToTheCaller)
{
    halWillCreateSession();
    EXPECT_CALL(m_player, configureCaptureSink(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*m_admission, admit(_)).WillOnce(Return(true));
    EXPECT_TRUE(run());
    EXPECT_EQ(m_committed.session.get(), m_session);
    EXPECT_EQ(m_committed.sessionId, kSessionId);
    EXPECT_EQ(m_committed.captureId, kCaptureId);
    EXPECT_TRUE(m_events.empty());
    m_committed.session.reset();
}
