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

#include "GLVideoFrameCapture.h"
#include "MediaPipelineTestBase.h"
#include "VideoFrameCaptureIpcMock.h"

using firebolt::rialto::client::GLVideoFrameCapture;
using firebolt::rialto::client::VideoFrameCaptureIpcMock;
using ::testing::StrictMock;

namespace
{
constexpr std::uint32_t kCaptureId{12};

// An application's own implementation, not created by Rialto's factory.
class ForeignCapture : public firebolt::rialto::IVideoFrameCapture
{
public:
    firebolt::rialto::GraphicsAPIType apiType() const override { return firebolt::rialto::GraphicsAPIType::GL; }
};
} // namespace

class RialtoClientMediaPipelineAttachCaptureTest : public MediaPipelineTestBase
{
protected:
    void SetUp() override
    {
        MediaPipelineTestBase::SetUp();
        createMediaPipeline();
        m_capture = std::make_shared<GLVideoFrameCapture>(m_captureIpcMock, kCaptureId, nullptr, nullptr);
    }

    void TearDown() override
    {
        if (m_capture)
        {
            expectCaptureDestroyed();
            m_capture.reset();
        }
        destroyMediaPipeline();
        MediaPipelineTestBase::TearDown();
    }

    void expectCaptureDestroyed()
    {
        EXPECT_CALL(*m_captureIpcMock, destroyVideoFrameCapture(kCaptureId)).WillOnce(Return(true));
    }

    void attach()
    {
        EXPECT_CALL(*m_mediaPipelineIpcMock, attachVideoFrameCapture(kCaptureId)).WillOnce(Return(true));
        ASSERT_TRUE(m_mediaPipeline->attachVideoFrameCapture(m_capture));
    }

    std::shared_ptr<StrictMock<VideoFrameCaptureIpcMock>> m_captureIpcMock{
        std::make_shared<StrictMock<VideoFrameCaptureIpcMock>>()};
    std::shared_ptr<GLVideoFrameCapture> m_capture;
};

TEST_F(RialtoClientMediaPipelineAttachCaptureTest, AttachAndDetach)
{
    attach();
    EXPECT_CALL(*m_mediaPipelineIpcMock, detachVideoFrameCapture()).WillOnce(Return(true));
    EXPECT_TRUE(m_mediaPipeline->detachVideoFrameCapture());
}

TEST_F(RialtoClientMediaPipelineAttachCaptureTest, NullAndForeignCapturesAreRejectedLocally)
{
    EXPECT_FALSE(m_mediaPipeline->attachVideoFrameCapture(nullptr));
    EXPECT_FALSE(m_mediaPipeline->attachVideoFrameCapture(std::make_shared<ForeignCapture>()));
}

TEST_F(RialtoClientMediaPipelineAttachCaptureTest, ServerRefusalLeavesNothingAttached)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, attachVideoFrameCapture(kCaptureId)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->attachVideoFrameCapture(m_capture));
    EXPECT_FALSE(m_mediaPipeline->detachVideoFrameCapture()); // nothing attached: not sent
    attach();                                                 // the mark was cleared, so a retry works
}

TEST_F(RialtoClientMediaPipelineAttachCaptureTest, SecondCaptureIsRejectedLocally)
{
    attach();
    auto second = std::make_shared<GLVideoFrameCapture>(m_captureIpcMock, kCaptureId + 1, nullptr, nullptr);
    EXPECT_FALSE(m_mediaPipeline->attachVideoFrameCapture(second));
    EXPECT_CALL(*m_captureIpcMock, destroyVideoFrameCapture(kCaptureId + 1)).WillOnce(Return(true));
}

TEST_F(RialtoClientMediaPipelineAttachCaptureTest, CaptureAttachedElsewhereIsRejectedLocally)
{
    attach();
    EXPECT_FALSE(m_capture->markAttached()); // what a second pipeline's attach checks
}

TEST_F(RialtoClientMediaPipelineAttachCaptureTest, FailedDetachKeepsTheCapture)
{
    attach();
    EXPECT_CALL(*m_mediaPipelineIpcMock, detachVideoFrameCapture()).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->detachVideoFrameCapture());
    EXPECT_CALL(*m_mediaPipelineIpcMock, detachVideoFrameCapture()).WillOnce(Return(true));
    EXPECT_TRUE(m_mediaPipeline->detachVideoFrameCapture());
}

TEST_F(RialtoClientMediaPipelineAttachCaptureTest, PipelineHoldsTheCaptureUntilDetach)
{
    attach();
    std::weak_ptr<GLVideoFrameCapture> weak{m_capture};
    m_capture.reset(); // the application's last reference: no destroyVideoFrameCapture yet
    EXPECT_FALSE(weak.expired());

    EXPECT_CALL(*m_mediaPipelineIpcMock, detachVideoFrameCapture()).WillOnce(Return(true));
    expectCaptureDestroyed();
    EXPECT_TRUE(m_mediaPipeline->detachVideoFrameCapture());
    EXPECT_TRUE(weak.expired());
}

TEST_F(RialtoClientMediaPipelineAttachCaptureTest, PipelineDestructionReleasesAndUnmarksTheCapture)
{
    attach();
    destroyMediaPipeline();
    EXPECT_TRUE(m_capture->markAttached()); // free for another pipeline
    m_capture->clearAttached();
    createMediaPipeline(); // for TearDown
}
