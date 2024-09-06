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


TEST_F(GstCapabilitiesTest, getSupportedPropertiesWithPropertiesSupported)
{
    createSutWithNoDecoderAndNoSink();

    const GstElementFactoryListType kExpectedFactoryListType{
        GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO};
    GList *listOfFactories{nullptr};
    GstElementFactory *dummyFactory{nullptr};
    const GType kDummyType{3};
    GObjectClass dummyClass;
    memset(&dummyClass, 0x00, sizeof(dummyClass));

    listOfFactories = g_list_append(listOfFactories, dummyFactory);
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(kExpectedFactoryListType, GST_RANK_NONE))
        .WillOnce(Return(listOfFactories));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetElementType(dummyFactory)).WillOnce(Return(kDummyType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kDummyType)).WillOnce(Return(&dummyClass));
    EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&dummyClass));

    // Params suppoted by the sink...
    const int kNumParamsSupportedByServer{2};
    GParamSpec dummySinkParams[kNumParamsSupportedByServer];
    dummySinkParams[0].name = "test-name-123";
    dummySinkParams[1].name = "test2";
    GParamSpec *dummySinkParamsPtr[] = {&dummySinkParams[0], &dummySinkParams[1]};

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassListProperties(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(kNumParamsSupportedByServer), Return(dummySinkParamsPtr)));
    EXPECT_CALL(*m_glibWrapperMock, gFree(dummySinkParamsPtr)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listOfFactories)).Times(1);

    // Params that the caller is asking about...
    std::vector<std::string> kParamNames{"test-name-123", "test2"};
    std::vector<std::string> supportedProperties{m_sut->getSupportedProperties(MediaSourceType::VIDEO, kParamNames)};
    // this time we should find all the properties...
    EXPECT_EQ(supportedProperties, kParamNames);

    gst_plugin_feature_list_free(listOfFactories);
}

TEST_F(GstCapabilitiesTest, getSupportedPropertiesWithPropertiesSupported_usePluginFeatureLoad)
{
    createSutWithNoDecoderAndNoSink();

    const GstElementFactoryListType kExpectedFactoryListType{
        GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO};
    GList *listOfFactories{nullptr};
    GstElementFactory *dummyFactory{nullptr};
    const GType kDummyType{3};
    GObjectClass dummyClass;
    memset(&dummyClass, 0x00, sizeof(dummyClass));

    listOfFactories = g_list_append(listOfFactories, dummyFactory);
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(kExpectedFactoryListType, GST_RANK_NONE))
        .WillOnce(Return(listOfFactories));

    // The next calls will ensure that gstPluginFeatureLoad is used
    char tmpFeatureAddress{1};
    GstPluginFeature *feature{reinterpret_cast<GstPluginFeature *>(tmpFeatureAddress)};
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureLoad(_)).WillOnce(Return(feature));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(feature));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetElementType(dummyFactory))
        .WillOnce(Return(G_TYPE_INVALID))
        .WillOnce(Return(kDummyType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kDummyType)).WillOnce(Return(&dummyClass));
    EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&dummyClass));

    // Params suppoted by the sink...
    const int kNumParamsSupportedByServer{1};
    GParamSpec dummySinkParams[kNumParamsSupportedByServer];
    dummySinkParams[0].name = "test-name-123";
    GParamSpec *dummySinkParamsPtr[] = {&dummySinkParams[0], &dummySinkParams[1]};

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassListProperties(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(kNumParamsSupportedByServer), Return(dummySinkParamsPtr)));
    EXPECT_CALL(*m_glibWrapperMock, gFree(dummySinkParamsPtr)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listOfFactories)).Times(1);

    // Params that the caller is asking about...
    std::vector<std::string> kParamNames{"test-name-123"};
    std::vector<std::string> supportedProperties{m_sut->getSupportedProperties(MediaSourceType::VIDEO, kParamNames)};
    // this time we should find all the properties...
    EXPECT_EQ(supportedProperties, kParamNames);

    gst_plugin_feature_list_free(listOfFactories);
}

TEST_F(GstCapabilitiesTest, getSupportedPropertiesWithPropertiesSupported_useObjectCreation)
{
    createSutWithNoDecoderAndNoSink();

    const GstElementFactoryListType kExpectedFactoryListType{
        GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO};
    GList *listOfFactories{nullptr};
    GstElementFactory *dummyFactory{nullptr};
    GObjectClass dummyClass;
    memset(&dummyClass, 0x00, sizeof(dummyClass));

    listOfFactories = g_list_append(listOfFactories, dummyFactory);
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(kExpectedFactoryListType, GST_RANK_NONE))
        .WillOnce(Return(listOfFactories));

    // The next calls should ensure that gstPluginFeatureLoad fails
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureLoad(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetElementType(dummyFactory)).WillOnce(Return(G_TYPE_INVALID));

    // The next calls should ensure that an object is created
    GstElement object;
    memset(&object, 0x00, sizeof(object));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(dummyFactory, nullptr)).WillOnce(Return(&object));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&object));

    // Params suppoted by the sink...
    const int kNumParamsSupportedByServer{1};
    GParamSpec dummySinkParams[kNumParamsSupportedByServer];
    dummySinkParams[0].name = "test-name-123";
    GParamSpec *dummySinkParamsPtr[] = {&dummySinkParams[0], &dummySinkParams[1]};

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassListProperties(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(kNumParamsSupportedByServer), Return(dummySinkParamsPtr)));
    EXPECT_CALL(*m_glibWrapperMock, gFree(dummySinkParamsPtr)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listOfFactories)).Times(1);

    // Params that the caller is asking about...
    std::vector<std::string> kParamNames{"test-name-123"};
    std::vector<std::string> supportedProperties{m_sut->getSupportedProperties(MediaSourceType::VIDEO, kParamNames)};
    // this time we should find all the properties...
    EXPECT_EQ(supportedProperties, kParamNames);

    gst_plugin_feature_list_free(listOfFactories);
}

TEST_F(GstCapabilitiesTest, getSupportedPropertiesWithPropertiesSupported_usePluginFeatureLoadAfterNoProperties)
{
    createSutWithNoDecoderAndNoSink();

    const GstElementFactoryListType kExpectedFactoryListType{
        GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO};
    GList *listOfFactories{nullptr};
    GstElementFactory *dummyFactory{nullptr};
    const GType kDummyType{3};
    GObjectClass dummyClass;
    memset(&dummyClass, 0x00, sizeof(dummyClass));

    listOfFactories = g_list_append(listOfFactories, dummyFactory);
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(kExpectedFactoryListType, GST_RANK_NONE))
        .WillOnce(Return(listOfFactories));

    // The next calls will ensure that gstPluginFeatureLoad is used AFTER a class was seen with no properties
    char tmpFeatureAddress{1};
    GstPluginFeature *feature{reinterpret_cast<GstPluginFeature *>(tmpFeatureAddress)};
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureLoad(_)).WillOnce(Return(feature));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(feature));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetElementType(dummyFactory)).WillRepeatedly(Return(kDummyType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kDummyType)).WillRepeatedly(Return(&dummyClass));
    EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&dummyClass)).Times(2);

    // Params suppoted by the sink...
    const int kNumParamsSupportedByServer{1};
    GParamSpec dummySinkParams[kNumParamsSupportedByServer];
    dummySinkParams[0].name = "test-name-123";
    GParamSpec *dummySinkParamsPtr[] = {&dummySinkParams[0], &dummySinkParams[1]};

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassListProperties(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(0), Return(nullptr)))
        .WillOnce(DoAll(SetArgPointee<1>(kNumParamsSupportedByServer), Return(dummySinkParamsPtr)));
    EXPECT_CALL(*m_glibWrapperMock, gFree(dummySinkParamsPtr)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listOfFactories)).Times(1);

    // Params that the caller is asking about...
    std::vector<std::string> kParamNames{"test-name-123"};
    std::vector<std::string> supportedProperties{m_sut->getSupportedProperties(MediaSourceType::VIDEO, kParamNames)};
    // this time we should find all the properties...
    EXPECT_EQ(supportedProperties, kParamNames);

    gst_plugin_feature_list_free(listOfFactories);
}

TEST_F(GstCapabilitiesTest, getSupportedPropertiesWithPropertiesSupported_useObjectCreationAfterNoProperties)
{
    createSutWithNoDecoderAndNoSink();

    const GstElementFactoryListType kExpectedFactoryListType{
        GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO};
    GList *listOfFactories{nullptr};
    GstElementFactory *dummyFactory{nullptr};
    const GType kDummyType{3};
    GObjectClass dummyClass;
    memset(&dummyClass, 0x00, sizeof(dummyClass));

    listOfFactories = g_list_append(listOfFactories, dummyFactory);
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(kExpectedFactoryListType, GST_RANK_NONE))
        .WillOnce(Return(listOfFactories));

    // The next calls will ensure that gstPluginFeatureLoad is used AFTER a class was seen with no properties
    char tmpFeatureAddress{1};
    GstPluginFeature *feature{reinterpret_cast<GstPluginFeature *>(tmpFeatureAddress)};
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureLoad(_)).WillOnce(Return(feature));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(feature));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetElementType(dummyFactory)).WillRepeatedly(Return(kDummyType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kDummyType)).WillRepeatedly(Return(&dummyClass));
    EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&dummyClass)).Times(2);

    // The next calls should ensure that an object is created
    GstElement object;
    memset(&object, 0x00, sizeof(object));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(dummyFactory, nullptr)).WillOnce(Return(&object));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&object));

    // Params suppoted by the sink...
    const int kNumParamsSupportedByServer{1};
    GParamSpec dummySinkParams[kNumParamsSupportedByServer];
    dummySinkParams[0].name = "test-name-123";
    GParamSpec *dummySinkParamsPtr[] = {&dummySinkParams[0], &dummySinkParams[1]};

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassListProperties(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(0), Return(nullptr)))
        .WillOnce(DoAll(SetArgPointee<1>(0), Return(nullptr)))
        .WillOnce(DoAll(SetArgPointee<1>(kNumParamsSupportedByServer), Return(dummySinkParamsPtr)));
    EXPECT_CALL(*m_glibWrapperMock, gFree(dummySinkParamsPtr)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listOfFactories)).Times(1);

    // Params that the caller is asking about...
    std::vector<std::string> kParamNames{"test-name-123"};
    std::vector<std::string> supportedProperties{m_sut->getSupportedProperties(MediaSourceType::VIDEO, kParamNames)};
    // this time we should find all the properties...
    EXPECT_EQ(supportedProperties, kParamNames);

    gst_plugin_feature_list_free(listOfFactories);
}

TEST_F(GstCapabilitiesTest, getSupportedPropertiesWithNoPropertiesSupported)
{
    createSutWithNoDecoderAndNoSink();

    const GstElementFactoryListType kExpectedFactoryListType{
        GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO};
    GList *listOfFactories{nullptr};
    GstElementFactory *dummyFactory{nullptr};
    const GType kDummyType{3};
    GObjectClass dummyClass;
    memset(&dummyClass, 0x00, sizeof(dummyClass));

    listOfFactories = g_list_append(listOfFactories, dummyFactory);
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(kExpectedFactoryListType, GST_RANK_NONE))
        .WillOnce(Return(listOfFactories));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetElementType(dummyFactory)).WillOnce(Return(kDummyType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kDummyType)).WillOnce(Return(&dummyClass));
    EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&dummyClass));

    // Params suppoted by the sink...
    const int kNumParamsSupportedByServer{2};
    GParamSpec dummySinkParams[kNumParamsSupportedByServer];
    dummySinkParams[0].name = "test3";
    dummySinkParams[1].name = "test4";
    GParamSpec *dummySinkParamsPtr[] = {&dummySinkParams[0], &dummySinkParams[1]};

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassListProperties(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(kNumParamsSupportedByServer), Return(dummySinkParamsPtr)));
    EXPECT_CALL(*m_glibWrapperMock, gFree(dummySinkParamsPtr)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listOfFactories)).Times(1);

    // Params that the caller is asking about...
    std::vector<std::string> kParamNames{"test-name-123", "test2"};
    std::vector<std::string> supportedProperties{m_sut->getSupportedProperties(MediaSourceType::VIDEO, kParamNames)};
    // this time we will not find the properties...
    EXPECT_TRUE(supportedProperties.empty());

    gst_plugin_feature_list_free(listOfFactories);
}

