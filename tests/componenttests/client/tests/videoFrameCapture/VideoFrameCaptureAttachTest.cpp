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

#include "ClientComponentTest.h"
#include "IVideoFrameCapture.h"
#include <gtest/gtest.h>

namespace firebolt::rialto::client::ct
{
namespace
{
constexpr std::uint32_t kFirstCaptureId{3};
constexpr std::uint32_t kSecondCaptureId{4};
constexpr int32_t kPrimarySessionId{10};
constexpr int32_t kSecondarySessionId{11};
} // namespace

/**
 * Client-side binding rules (design D2): one capture per pipeline and one pipeline per capture, checked locally.
 */
class VideoFrameCaptureAttachTest : public ClientComponentTest
{
public:
    VideoFrameCaptureAttachTest() : ClientComponentTest()
    {
        enableVideoFrameCaptureModule();
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::shouldCreateMediaSession();
        MediaPipelineTestMethods::createMediaPipeline();
        MediaPipelineTestMethods::shouldCreateMediaSessionSecondary();
        MediaPipelineTestMethods::createMediaPipelineSecondary();
    }

    ~VideoFrameCaptureAttachTest()
    {
        MediaPipelineTestMethods::shouldDestroyMediaSession();
        MediaPipelineTestMethods::destroyMediaPipeline();
        MediaPipelineTestMethods::shouldDestroyMediaSessionSecondary();
        MediaPipelineTestMethods::destroyMediaPipelineSecondary();
        ClientComponentTest::stopApplication();
    }

    std::shared_ptr<IGLVideoFrameCapture> createCapture(std::uint32_t captureId)
    {
        EXPECT_CALL(*m_videoFrameCaptureModuleMock, createVideoFrameCapture(_, _, _, _))
            .WillOnce(Invoke(
                [captureId](::google::protobuf::RpcController *, const CreateVideoFrameCaptureRequest *,
                            CreateVideoFrameCaptureResponse *response, ::google::protobuf::Closure *done)
                {
                    response->set_status(VIDEO_FRAME_CAPTURE_STATUS_OK);
                    response->set_capture_id(captureId);
                    done->Run();
                }));
        auto capture = IVideoFrameCaptureFactory::createFactory()->createGLVideoFrameCapture(nullptr, nullptr);
        EXPECT_NE(capture, nullptr);
        return capture;
    }

    void serverWillAttach(int32_t sessionId, std::uint32_t captureId)
    {
        EXPECT_CALL(*m_mediaPipelineModuleMock, attachVideoFrameCapture(_, _, _, _))
            .WillOnce(Invoke(
                [sessionId, captureId](::google::protobuf::RpcController *, const AttachVideoFrameCaptureRequest *request,
                                       AttachVideoFrameCaptureResponse *, ::google::protobuf::Closure *done)
                {
                    EXPECT_EQ(request->session_id(), sessionId);
                    EXPECT_EQ(request->capture_id(), captureId);
                    done->Run();
                }))
            .RetiresOnSaturation();
    }

    void serverWillDetach(int32_t sessionId)
    {
        EXPECT_CALL(*m_mediaPipelineModuleMock, detachVideoFrameCapture(_, _, _, _))
            .WillOnce(Invoke(
                [sessionId](::google::protobuf::RpcController *, const DetachVideoFrameCaptureRequest *request,
                            DetachVideoFrameCaptureResponse *, ::google::protobuf::Closure *done)
                {
                    EXPECT_EQ(request->session_id(), sessionId);
                    done->Run();
                }))
            .RetiresOnSaturation();
    }

    void serverWillDestroyCapture(std::uint32_t captureId)
    {
        EXPECT_CALL(*m_videoFrameCaptureModuleMock, destroyVideoFrameCapture(_, _, _, _))
            .WillOnce(Invoke(
                [captureId](::google::protobuf::RpcController *, const DestroyVideoFrameCaptureRequest *request,
                            DestroyVideoFrameCaptureResponse *, ::google::protobuf::Closure *done)
                {
                    EXPECT_EQ(request->capture_id(), captureId);
                    done->Run();
                }))
            .RetiresOnSaturation();
    }
};

/*
 * Component Test: One capture, two pipelines
 * Test Objective:
 *  Check that a capture attached to one pipeline cannot be attached to another until it is detached.
 *
 * Test Steps:
 *  Step 1: Attach the capture to the primary pipeline.
 *  Step 2: Attach it to the secondary pipeline: refused without a server call.
 *  Step 3: Detach it from the primary pipeline and attach it to the secondary one.
 *
 * Expected Results:
 *  The capture is on one pipeline at a time and is reusable after detach.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAttachTest, oneCaptureOnTwoPipelines)
{
    auto capture = createCapture(kFirstCaptureId);

    // Step 1: Attach to the primary pipeline
    serverWillAttach(kPrimarySessionId, kFirstCaptureId);
    EXPECT_TRUE(m_mediaPipeline->attachVideoFrameCapture(capture));

    // Step 2: The secondary pipeline is refused locally
    EXPECT_FALSE(m_mediaPipelineSecondary->attachVideoFrameCapture(capture));

    // Step 3: Reuse after detach
    serverWillDetach(kPrimarySessionId);
    EXPECT_TRUE(m_mediaPipeline->detachVideoFrameCapture());
    serverWillAttach(kSecondarySessionId, kFirstCaptureId);
    EXPECT_TRUE(m_mediaPipelineSecondary->attachVideoFrameCapture(capture));

    serverWillDetach(kSecondarySessionId);
    EXPECT_TRUE(m_mediaPipelineSecondary->detachVideoFrameCapture());
    serverWillDestroyCapture(kFirstCaptureId);
    capture.reset();
}

/*
 * Component Test: Two captures, one pipeline
 * Test Objective:
 *  Check that a pipeline takes one capture at a time.
 *
 * Test Steps:
 *  Step 1: Attach the first capture.
 *  Step 2: Attach the second: refused without a server call; the first stays attached.
 *  Step 3: Detach the first and attach the second.
 *
 * Expected Results:
 *  The pipeline has at most one capture.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAttachTest, twoCapturesOnOnePipeline)
{
    auto first = createCapture(kFirstCaptureId);
    auto second = createCapture(kSecondCaptureId);

    // Step 1: Attach the first capture
    serverWillAttach(kPrimarySessionId, kFirstCaptureId);
    EXPECT_TRUE(m_mediaPipeline->attachVideoFrameCapture(first));

    // Step 2: The second capture is refused
    EXPECT_FALSE(m_mediaPipeline->attachVideoFrameCapture(second));

    // Step 3: Swap captures
    serverWillDetach(kPrimarySessionId);
    EXPECT_TRUE(m_mediaPipeline->detachVideoFrameCapture());
    serverWillAttach(kPrimarySessionId, kSecondCaptureId);
    EXPECT_TRUE(m_mediaPipeline->attachVideoFrameCapture(second));

    // the pipeline keeps the second capture alive; it is destroyed with the pipeline
    serverWillDestroyCapture(kFirstCaptureId);
    first.reset();
    second.reset();
    serverWillDestroyCapture(kSecondCaptureId);
}
} // namespace firebolt::rialto::client::ct
