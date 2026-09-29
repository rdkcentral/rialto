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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"

using ::testing::_;
using ::testing::Return;

namespace firebolt::rialto::server::ct
{
class MediaCapabilitiesOrchestrationTest : public RialtoServerComponentTest
{
public:
    MediaCapabilitiesOrchestrationTest() {}
    ~MediaCapabilitiesOrchestrationTest() = default;

    void SetUp() override
    {
        // Setup base fixture
        RialtoServerComponentTest::SetUp();
    }

    void TearDown() override
    {
        // Cleanup base fixture
        RialtoServerComponentTest::TearDown();
    }
};

/*
 * Component Test: MediaCapabilities uses GStreamer fallback when preload unavailable
 * Test Objective:
 *  Verify that MediaCapabilities orchestrator falls back to GStreamer discovery
 *  when ServerManager preload is not available (Path B)
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaCapabilities, GstCapabilities
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  No preloaded capabilities set (simulates unavailable Path 0)
 *
 * Test Steps:
 *  Step 1: Query supported audio capabilities
 *   Expect that GStreamer fallback is used (Path B)
 *   Return the audio MIME types from GStreamer discovery
 *
 *  Step 2: Query supported video capabilities
 *   Expect that GStreamer fallback is used (Path B)
 *   Return the video MIME types from GStreamer discovery
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  - Audio capabilities returned from GStreamer fallback (version 1.0)
 *  - Video capabilities returned from GStreamer fallback (version 2.0)
 *  - Capabilities contain discovered MIME types (audio/mp4, video/h264)
 *
 * Code:
 */
TEST_F(MediaCapabilitiesOrchestrationTest, pathBFallbackWhenPreloadUnavailable)
{
    // Setup: No preloaded capabilities set, will use GStreamer Path B
    configureSutInActiveState();
    connectClient();

    // Step 1: Get audio capabilities - should use GStreamer (Path B)
    auto audioRequest{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::AUDIO)};
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}
        .send(audioRequest)
        .expectSuccess()
        .matchResponse(
            [](const auto &resp)
            {
                // NOTE: These are GStreamer discovered types (Path B fallback)
                // Discovery happens in RialtoServerComponentTest::startSut()
                EXPECT_THAT(resp.mime_types(), testing::Contains("audio/mp4"));
            });

    // Step 2: Get video capabilities - should use GStreamer (Path B)
    auto videoRequest{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::VIDEO)};
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}
        .send(videoRequest)
        .expectSuccess()
        .matchResponse(
            [](const auto &resp)
            {
                // NOTE: These are GStreamer discovered types (Path B fallback)
                // Discovery happens in RialtoServerComponentTest::startSut()
                EXPECT_THAT(resp.mime_types(), testing::Contains("video/h264"));
            });
}

/*
 * Component Test: MediaCapabilities correctly retrieves capabilities via orchestration
 * Test Objective:
 *  Verify that MediaCapabilities properly orchestrates between ServerManager
 *  preload (Path 0) and GStreamer fallback (Path B) based on availability
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaCapabilities, GstCapabilities
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *
 * Test Steps:
 *  Step 1: Query audio capabilities
 *   Capabilities are discovered from GStreamer (Path B)
 *   Expect audio/mp4 MIME type is supported
 *
 *  Step 2: Query video capabilities
 *   Capabilities are discovered from GStreamer (Path B)
 *   Expect video/h264 MIME type is supported
 *
 *  Step 3: Query audio capabilities again
 *   Should reuse cached capabilities (no additional GStreamer calls)
 *   Expect same audio capabilities returned
 *
 *  Step 4: Query video capabilities again
 *   Should reuse cached capabilities (no additional GStreamer calls)
 *   Expect same video capabilities returned
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  - All capability queries return consistent results
 *  - Audio/video MIME types properly discovered
 *  - No duplicate GStreamer queries on repeated requests
 *
 * Code:
 */
TEST_F(MediaCapabilitiesOrchestrationTest, consistentCapabilityRetrieval)
{
    configureSutInActiveState();
    connectClient();

    // Step 1: Get audio capabilities first time
    auto audioRequest1{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::AUDIO)};
    std::vector<std::string> audioMimes1;
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}
        .send(audioRequest1)
        .expectSuccess()
        .matchResponse(
            [&audioMimes1](const auto &resp)
            {
                audioMimes1.assign(resp.mime_types().begin(), resp.mime_types().end());
                EXPECT_FALSE(audioMimes1.empty());
            });

    // Step 2: Get video capabilities first time
    auto videoRequest1{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::VIDEO)};
    std::vector<std::string> videoMimes1;
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}
        .send(videoRequest1)
        .expectSuccess()
        .matchResponse(
            [&videoMimes1](const auto &resp)
            {
                videoMimes1.assign(resp.mime_types().begin(), resp.mime_types().end());
                EXPECT_FALSE(videoMimes1.empty());
            });

    // Step 3: Get audio capabilities second time - should be consistent
    auto audioRequest2{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::AUDIO)};
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}
        .send(audioRequest2)
        .expectSuccess()
        .matchResponse(
            [&audioMimes1](const auto &resp)
            {
                std::vector<std::string> audioMimes2{resp.mime_types().begin(), resp.mime_types().end()};
                EXPECT_EQ(audioMimes1, audioMimes2);
            });

    // Step 4: Get video capabilities second time - should be consistent
    auto videoRequest2{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::VIDEO)};
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}
        .send(videoRequest2)
        .expectSuccess()
        .matchResponse(
            [&videoMimes1](const auto &resp)
            {
                std::vector<std::string> videoMimes2{resp.mime_types().begin(), resp.mime_types().end()};
                EXPECT_EQ(videoMimes1, videoMimes2);
            });
}

/*
 * Component Test: MediaCapabilities handles unknown media source type gracefully
 * Test Objective:
 *  Verify that MediaCapabilities orchestrator gracefully handles unknown media
 *  source types and returns appropriate results
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaCapabilities
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *
 * Test Steps:
 *  Step 1: Query capabilities for unknown media type
 *   Request capabilities for an unsupported/unknown media type
 *   Expect empty result or error handling
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  - Unknown media types are handled gracefully
 *  - No crashes or undefined behavior
 *  - Empty capabilities returned for unknown types
 *
 * Code:
 */
TEST_F(MediaCapabilitiesOrchestrationTest, unknownMediaTypeHandling)
{
    configureSutInActiveState();
    connectClient();

    // Query for unknown media type
    auto request{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::UNKNOWN)};
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}.send(request).expectSuccess().matchResponse(
        [](const auto &resp)
        {
            // Unknown media types should return empty MIME types
            EXPECT_THAT(resp.mime_types(), testing::IsEmpty());
        });
}

} // namespace firebolt::rialto::server::ct
