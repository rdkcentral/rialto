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


    void willCallGetSupportedProperties()
    {
        const GType kDummyType{3};
        GstElementFactory *dummyFactory{nullptr};
        memset(&m_dummyClass, 0x00, sizeof(m_dummyClass));
        GstElementFactoryListType expectedFactoryListType{
            GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO};
        m_listOfFactories = g_list_append(m_listOfFactories, dummyFactory);
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(expectedFactoryListType, GST_RANK_NONE))
            .WillOnce(Return(m_listOfFactories));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetElementType(dummyFactory)).WillOnce(Return(kDummyType));
        EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kDummyType)).WillOnce(Return(&m_dummyClass));
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassListProperties(_, _))
            .WillRepeatedly(DoAll(SetArgPointee<1>(kNumPropertiesOnSink), Return(m_dummyParamsPtr)));
        EXPECT_CALL(*m_glibWrapperMock, gFree(m_dummyParamsPtr)).Times(1);
        EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&m_dummyClass));
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
    GObjectClass m_dummyClass;
    GParamSpec m_dummyParams[kNumPropertiesOnSink];
    GParamSpec *m_dummyParamsPtr[kNumPropertiesOnSink];
    std::vector<std::string> m_kParamNames{kPropertyName1, kPropertyName2};
};


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
