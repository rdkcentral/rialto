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
constexpr std::uint32_t kCaptureId{3};
} // namespace

class VideoFrameCaptureRegistrationTest : public ClientComponentTest
{
public:
    VideoFrameCaptureRegistrationTest() : ClientComponentTest()
    {
        enableVideoFrameCaptureModule();
        ClientComponentTest::startApplicationRunning();
    }

    ~VideoFrameCaptureRegistrationTest() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: Create and destroy a GL capture
 * Test Objective:
 *  Check that creating a GL capture registers it with the server and that dropping it unregisters it.
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: IVideoFrameCaptureFactory, VideoFrameCaptureModule
 *
 * Test Steps:
 *  Step 1: Create a GL capture
 *   Expect a createVideoFrameCapture call for the GL API, answered with capture ID 3.
 *   Expect a capture whose apiType() is GL.
 *
 *  Step 2: Drop the capture
 *   Expect a destroyVideoFrameCapture call for capture ID 3.
 *
 * Expected Results:
 *  The capture is registered and unregistered with the server.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureRegistrationTest, createAndDestroyGlCapture)
{
    // Step 1: Create a GL capture
    EXPECT_CALL(*m_videoFrameCaptureModuleMock, createVideoFrameCapture(_, _, _, _))
        .WillOnce(Invoke(
            [](::google::protobuf::RpcController *, const CreateVideoFrameCaptureRequest *request,
               CreateVideoFrameCaptureResponse *response, ::google::protobuf::Closure *done)
            {
                EXPECT_EQ(request->api(), GRAPHICS_API_TYPE_GL);
                response->set_status(VIDEO_FRAME_CAPTURE_STATUS_OK);
                response->set_capture_id(kCaptureId);
                done->Run();
            }));
    std::shared_ptr<IGLVideoFrameCapture> capture =
        IVideoFrameCaptureFactory::createFactory()->createGLVideoFrameCapture(nullptr, nullptr);
    ASSERT_NE(capture, nullptr);
    EXPECT_EQ(capture->apiType(), GraphicsAPIType::GL);

    // Step 2: Drop the capture
    EXPECT_CALL(*m_videoFrameCaptureModuleMock, destroyVideoFrameCapture(_, _, _, _))
        .WillOnce(Invoke(
            [](::google::protobuf::RpcController *, const DestroyVideoFrameCaptureRequest *request,
               DestroyVideoFrameCaptureResponse *, ::google::protobuf::Closure *done)
            {
                EXPECT_EQ(request->capture_id(), kCaptureId);
                done->Run();
            }));
    capture.reset();
}

/*
 * Component Test: The server does not register the capture
 * Expected Results:
 *  An UNSUPPORTED answer gives no capture.
 */
TEST_F(VideoFrameCaptureRegistrationTest, noCaptureWhenServerReportsUnsupported)
{
    EXPECT_CALL(*m_videoFrameCaptureModuleMock, createVideoFrameCapture(_, _, _, _))
        .WillOnce(Invoke(
            [](::google::protobuf::RpcController *, const CreateVideoFrameCaptureRequest *,
               CreateVideoFrameCaptureResponse *response, ::google::protobuf::Closure *done)
            {
                response->set_status(VIDEO_FRAME_CAPTURE_STATUS_UNSUPPORTED);
                done->Run();
            }));
    EXPECT_EQ(IVideoFrameCaptureFactory::createFactory()->createGLVideoFrameCapture(nullptr, nullptr), nullptr);
}

class VideoFrameCaptureModuleDisabledTest : public ClientComponentTest
{
public:
    VideoFrameCaptureModuleDisabledTest() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }
    ~VideoFrameCaptureModuleDisabledTest() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: The server has no capture module
 * Expected Results:
 *  Creating a GL capture returns null.
 */
TEST_F(VideoFrameCaptureModuleDisabledTest, noCaptureWithoutServerModule)
{
    EXPECT_EQ(IVideoFrameCaptureFactory::createFactory()->createGLVideoFrameCapture(nullptr, nullptr), nullptr);
}
} // namespace firebolt::rialto::client::ct
