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
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;

namespace
{
constexpr int kNumPropertiesOnSink{3};
const char *kPropertyName1 = "test-name-5";
const char *kPropertyName2 = "test2";
const char *kPropertyName3 = "prop";
const char *kAudioFade = "audio-fade";
const GstElementFactoryListType kExpectedFactoryListType{
    GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO};
}; // namespace
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

        m_dummyParams[0].name = kPropertyName1;
        m_dummyParams[1].name = kPropertyName3;
        m_dummyParams[2].name = kPropertyName2;
        for (int i = 0; i < kNumPropertiesOnSink; ++i)
            m_dummyParamsPtr[i] = &m_dummyParams[i];
        memset(&m_object, 0x00, sizeof(m_object));
        m_elementFactory = gst_element_factory_find("fakesrc");
    }
    ~MediaPipelineCapabilitiesTest() { gst_object_unref(m_elementFactory); }

    void willCallGetSupportedProperties()
    {
        m_listOfFactories = g_list_append(m_listOfFactories, m_elementFactory);
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(kExpectedFactoryListType, GST_RANK_NONE))
            .WillOnce(Return(m_listOfFactories));
        // The next calls should ensure that an object is created and then freed
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(m_elementFactory, nullptr)).WillOnce(Return(&m_object));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_object));

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassListProperties(_, _))
            .WillRepeatedly(DoAll(SetArgPointee<1>(kNumPropertiesOnSink), Return(m_dummyParamsPtr)));
        EXPECT_CALL(*m_rdkGstreamerUtilsWrapperMock, isSocAudioFadeSupported()).WillOnce(Return(true));
        EXPECT_CALL(*m_glibWrapperMock, gFree(m_dummyParamsPtr)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(m_listOfFactories)).Times(1);
    }

    void callGetSupportedProperties()
    {
        auto request{createGetSupportedPropertiesRequest(ProtoMediaSourceType::VIDEO, m_kParamNames)};
        ConfigureAction<GetSupportedProperties>{m_clientStub}.send(request).expectSuccess().matchResponse(
            [this](const auto &resp)
            {
                std::vector<std::string> supportedProperties{resp.supported_properties().begin(),
                                                             resp.supported_properties().end()};
                EXPECT_EQ(supportedProperties, m_kParamNames);
            });

        gst_plugin_feature_list_free(m_listOfFactories);
        m_listOfFactories = nullptr;
    }

private:
    GList *m_listOfFactories{nullptr};
    GParamSpec m_dummyParams[kNumPropertiesOnSink];
    GParamSpec *m_dummyParamsPtr[kNumPropertiesOnSink];
    std::vector<std::string> m_kParamNames{kPropertyName1, kPropertyName3, kPropertyName2, kAudioFade};
    GstElement m_object;
    GstElementFactory *m_elementFactory;
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
}

/*
 * Component Test: Check that the API call GetSupportedProperties works
 * Test Objective:
 *  Test that the RialtoServer can process a request to get supported pipeline properites
 *
 * Sequence Diagrams:
 *  Capabilities - Get Supported Properties
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-Getsupportedproperties
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
 *  Step 1: Get supported properties
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Rialto server checks, if mime types are supported.
 *
 * Code:
 */
TEST_F(MediaPipelineCapabilitiesTest, checkGetSupportedProperties)
{
    // Step 1: Get supported properties
    willCallGetSupportedProperties();
    callGetSupportedProperties();
}
} // namespace firebolt::rialto::server::ct
