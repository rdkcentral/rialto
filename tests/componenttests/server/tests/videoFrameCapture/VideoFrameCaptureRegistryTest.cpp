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

#include "ActionTraits.h"
#include "ClientStub.h"
#include "ConfigureAction.h"
#include "RialtoServerComponentTest.h"

#include <cstdint>
#include <cstdlib>

namespace firebolt::rialto::server::ct
{
namespace
{
constexpr const char *kCaptureHalLibraryEnv{"RIALTO_CAPTURE_HAL_LIBRARY"};
} // namespace

class VideoFrameCaptureRegistryTest : public RialtoServerComponentTest
{
public:
    VideoFrameCaptureRegistryTest()
    {
        // the capture HAL is loaded on the first create, so this applies to the server started below
        setenv(kCaptureHalLibraryEnv, FAKE_CAPTURE_HAL_PATH, 1);
        configureSutInActiveState();
        connectClient();
    }

    ~VideoFrameCaptureRegistryTest() override { unsetenv(kCaptureHalLibraryEnv); }

    std::uint32_t createCapture(IStub &client)
    {
        CreateVideoFrameCaptureRequest request;
        request.set_api(GRAPHICS_API_TYPE_GL);
        std::uint32_t captureId{0};
        ConfigureAction<CreateVideoFrameCapture>(client).send(request).expectSuccess().matchResponse(
            [&](const CreateVideoFrameCaptureResponse &response)
            {
                EXPECT_EQ(response.status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
                EXPECT_TRUE(response.has_capture_id());
                captureId = response.capture_id();
            });
        EXPECT_NE(captureId, 0U);
        return captureId;
    }

    void destroyCapture(IStub &client, std::uint32_t captureId, bool expectSuccess)
    {
        DestroyVideoFrameCaptureRequest request;
        request.set_capture_id(captureId);
        auto action = ConfigureAction<DestroyVideoFrameCapture>(client);
        action.send(request);
        if (expectSuccess)
            action.expectSuccess();
        else
            action.expectFailure();
    }
};

/*
 * Component Test: Video frame capture registration
 * Test Objective:
 *  Check that the server registers captures per client connection with the fake capture HAL available.
 *
 * Test Steps:
 *  Create a capture and destroy it; destroying it again fails.
 *
 * Expected Results:
 *  The capture gets a non-zero ID and can be destroyed exactly once.
 */
TEST_F(VideoFrameCaptureRegistryTest, shouldCreateAndDestroyCapture)
{
    const std::uint32_t kCaptureId = createCapture(m_clientStub);
    destroyCapture(m_clientStub, kCaptureId, true);
    destroyCapture(m_clientStub, kCaptureId, false);
}

/*
 * Component Test: Two captures on one connection
 * Expected Results:
 *  They get different IDs.
 */
TEST_F(VideoFrameCaptureRegistryTest, shouldGiveTwoCapturesDifferentIds)
{
    const std::uint32_t kFirst = createCapture(m_clientStub);
    const std::uint32_t kSecond = createCapture(m_clientStub);
    EXPECT_NE(kFirst, kSecond);
}

/*
 * Component Test: Capture IDs are scoped to their connection
 * Expected Results:
 *  A second client cannot destroy the first client's capture, which stays usable.
 */
TEST_F(VideoFrameCaptureRegistryTest, shouldRejectCaptureIdFromAnotherConnection)
{
    ClientStub otherClient;
    ASSERT_TRUE(otherClient.connect());

    const std::uint32_t kCaptureId = createCapture(m_clientStub);
    destroyCapture(otherClient, kCaptureId, false);
    destroyCapture(m_clientStub, kCaptureId, true);

    otherClient.disconnect();
}

/*
 * Component Test: Disconnecting removes a client's captures
 * Expected Results:
 *  A client with two captures disconnects without affecting another client's capture.
 */
TEST_F(VideoFrameCaptureRegistryTest, shouldRemoveCapturesOfDisconnectedClientOnly)
{
    const std::uint32_t kCaptureId = createCapture(m_clientStub);
    {
        ClientStub otherClient;
        ASSERT_TRUE(otherClient.connect());
        createCapture(otherClient);
        createCapture(otherClient);
        otherClient.disconnect();
    }

    destroyCapture(m_clientStub, kCaptureId, true);
}

class VideoFrameCaptureUnavailableTest : public RialtoServerComponentTest
{
public:
    VideoFrameCaptureUnavailableTest()
    {
        setenv(kCaptureHalLibraryEnv, "/nonexistent/lib_capture_hal.so", 1);
        configureSutInActiveState();
        connectClient();
    }

    ~VideoFrameCaptureUnavailableTest() override { unsetenv(kCaptureHalLibraryEnv); }
};

/*
 * Component Test: No capture HAL
 * Expected Results:
 *  Creating a capture answers UNSUPPORTED without an ID.
 */
TEST_F(VideoFrameCaptureUnavailableTest, shouldReportUnsupportedWithoutCaptureHal)
{
    CreateVideoFrameCaptureRequest request;
    request.set_api(GRAPHICS_API_TYPE_GL);
    ConfigureAction<CreateVideoFrameCapture>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [](const CreateVideoFrameCaptureResponse &response)
            {
                EXPECT_EQ(response.status(), VIDEO_FRAME_CAPTURE_STATUS_UNSUPPORTED);
                EXPECT_FALSE(response.has_capture_id());
            });
}
} // namespace firebolt::rialto::server::ct
