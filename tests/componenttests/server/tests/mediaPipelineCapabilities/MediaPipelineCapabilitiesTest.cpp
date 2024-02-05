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

using ::testing::UnorderedElementsAre;

namespace firebolt::rialto::server::ct
{
class MediaPipelineCapabilitiesTest : public RialtoServerComponentTest
{
public:
    MediaPipelineCapabilitiesTest()
    {
        willConfigureSocket();
        configureSutInActiveState();
        connectClient();
    }
    ~MediaPipelineCapabilitiesTest() override = default;
};

/*
 * Component Test: Get audio capabilities
 * Test Objective:
 *  Test if Rialto Server returns supported audio mime types
 *
 * Sequence Diagrams:
 *  Capabilities - Supported MIME types
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-SupportedMIMETypes
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipelineCapabilities
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *
 * Test Steps:
 *  Step 1: Get audio capabilities
 *   Client stub requests the server to get audio capabilities
 *   Expect that server returns supported audio capabilities
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Rialto server returns supported audio capabilities.
 *
 * Code:
 */
TEST_F(MediaPipelineCapabilitiesTest, getAudioCapabilities)
{
    // Step 1: Get audio capabilities
    auto request{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::AUDIO)};
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}.send(request).expectSuccess().matchResponse(
        [](const auto &resp)
        {
            // NOTE: Supported Mime types are initialized during SUT initialization, in RialtoServerComponentTest::startSut()
            // Returned mime types should intersect with "audio/mpeg, mpegversion=(int)4" static pad caps
            EXPECT_THAT(resp.mime_types(), UnorderedElementsAre("audio/x-eac3", "audio/aac", "audio/mp4"));
        });
};

/*
 * Component Test: Get video capabilities
 * Test Objective:
 *  Test if Rialto Server returns supported video mime types
 *
 * Sequence Diagrams:
 *  Capabilities - Supported MIME types
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-SupportedMIMETypes
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipelineCapabilities
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *
 * Test Steps:
 *  Step 1: Get video capabilities
 *   Client stub requests the server to get video capabilities
 *   Expect that server returns supported video capabilities
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Rialto server returns supported video capabilities.
 *
 * Code:
 */
TEST_F(MediaPipelineCapabilitiesTest, getVideoCapabilities)
{
    // Step 1: Get video capabilities
    auto request{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::VIDEO)};
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}.send(request).expectSuccess().matchResponse(
        [](const auto &resp)
        {
            // NOTE: Supported Mime types are initialized during SUT initialization, in RialtoServerComponentTest::startSut()
            // Returned mime types should intersect with "video/x-h264" static pad caps
            EXPECT_THAT(resp.mime_types(), UnorderedElementsAre("video/h264"));
        });
};

/*
 * Component Test: Get unknown capabilities
 * Test Objective:
 *  Test if Rialto Server returns empty vector for unknown capabilities
 *
 * Sequence Diagrams:
 *  Capabilities - Supported MIME types
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-SupportedMIMETypes
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipelineCapabilities
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *
 * Test Steps:
 *  Step 1: Get unknown capabilities
 *   Client stub requests the server to get unknown capabilities
 *   Expect that server returns empty vector
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Rialto server returns empty vector for unknwon capabilities.
 *
 * Code:
 */
TEST_F(MediaPipelineCapabilitiesTest, getUnknownCapabilities)
{
    // Step 1: Get unknown capabilities
    auto request{createGetSupportedMimeTypesRequest(ProtoMediaSourceType::UNKNOWN)};
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}.send(request).expectSuccess().matchResponse(
        [](const auto &resp) { EXPECT_TRUE(resp.mime_types().empty()); });
};

/*
 * Component Test: Check, if audio mime type is supported
 * Test Objective:
 *  Test if requested mime type is supported by Rialto Server
 *
 * Sequence Diagrams:
 *  Capabilities - Supported MIME types
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-SupportedMIMETypes
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipelineCapabilities
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *
 * Test Steps:
 *  Step 1: Check audio/mp4 mime type
 *   Client stub requests the server to check, if audio/mp4 mime type is supported
 *   Expect that server returns, that this mime type is supported
 *
 *  Step 2: Check audio/x-opus mime type
 *   Client stub requests the server to check, if audio/x-opus mime type is supported
 *   Expect that server returns, that this mime type is not supported
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Rialto server checks, if mime types are supported.
 *
 * Code:
 */
TEST_F(MediaPipelineCapabilitiesTest, checkAudioMimeTypes)
{
    // NOTE: Supported Mime types are initialized during SUT initialization, in RialtoServerComponentTest::startSut()
    // Returned mime types should intersect with "audio/mpeg, mpegversion=(int)4" static pad caps

    // Step 1: Check audio/mp4 mime type
    auto mp4Request{createIsMimeTypeSupportedRequest("audio/mp4")};
    ConfigureAction<IsMimeTypeSupported>{m_clientStub}
        .send(mp4Request)
        .expectSuccess()
        .matchResponse([](const auto &resp) { EXPECT_TRUE(resp.is_supported()); });

    // Step 2: Check audio/x-opus mime type
    auto opusRequest{createIsMimeTypeSupportedRequest("audio/x-opus")};
    ConfigureAction<IsMimeTypeSupported>{m_clientStub}
        .send(opusRequest)
        .expectSuccess()
        .matchResponse([](const auto &resp) { EXPECT_FALSE(resp.is_supported()); });
};

/*
 * Component Test: Check, if video mime type is supported
 * Test Objective:
 *  Test if requested mime type is supported by Rialto Server
 *
 * Sequence Diagrams:
 *  Capabilities - Supported MIME types
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-SupportedMIMETypes
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipelineCapabilities
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *
 * Test Steps:
 *  Step 1: Check video/h264 mime type
 *   Client stub requests the server to check, if video/h264 mime type is supported
 *   Expect that server returns, that this mime type is supported
 *
 *  Step 2: Check video/x-vp9 mime type
 *   Client stub requests the server to check, if video/x-vp9 mime type is supported
 *   Expect that server returns, that this mime type is not supported
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Rialto server checks, if mime types are supported.
 *
 * Code:
 */
TEST_F(MediaPipelineCapabilitiesTest, checkVideoMimeTypes)
{
    // NOTE: Supported Mime types are initialized during SUT initialization, in RialtoServerComponentTest::startSut()
    // Returned mime types should intersect with "audio/mpeg, mpegversion=(int)4" static pad caps

    // Step 1: Check video/h264 mime type
    auto h264Request{createIsMimeTypeSupportedRequest("video/h264")};
    ConfigureAction<IsMimeTypeSupported>{m_clientStub}
        .send(h264Request)
        .expectSuccess()
        .matchResponse([](const auto &resp) { EXPECT_TRUE(resp.is_supported()); });

    // Step 2: Check video/x-vp9 mime type
    auto vp9Request{createIsMimeTypeSupportedRequest("video/x-vp9")};
    ConfigureAction<IsMimeTypeSupported>{m_clientStub}
        .send(vp9Request)
        .expectSuccess()
        .matchResponse([](const auto &resp) { EXPECT_FALSE(resp.is_supported()); });
};
} // namespace firebolt::rialto::server::ct
