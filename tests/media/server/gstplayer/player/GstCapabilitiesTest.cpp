/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "GstWrapperMock.h"
#include <GstCapabilities.h>
#include <gtest/gtest.h>
#include <unordered_map>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;
using ::testing::UnorderedElementsAre;

class GstCapabilitiesTest : public testing::Test
{
public:
    GstCapabilitiesTest()
    {
        // valid values of GstCaps are not needed, only addresses will be used
        m_capsMap = {{"audio/mpeg, mpegversion=(int)4", {}},
                     {"audio/x-eac3", {}},
                     {"audio/x-opus", {}},
                     {"video/x-av1", {}},
                     {"video/x-h264", {}},
                     {"video/x-h265", {}},
                     {"video/x-vp9", {}}};
    }
    ~GstCapabilitiesTest() = default;

    void expectCapsToMimeMapping()
    {
        for (auto &capsMap : m_capsMap)
        {
            EXPECT_CALL(*m_gstWrapperMock, gstCapsFromString(StrEq(capsMap.first.c_str()))).WillOnce(Return(&capsMap.second));
            EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&capsMap.second)).Times(1);
        }
    }

    std::shared_ptr<StrictMock<mock::GstWrapperMock>> m_gstWrapperMock{
        std::make_shared<StrictMock<mock::GstWrapperMock>>()};
    std::unordered_map<std::string, GstCaps> m_capsMap;
    std::unique_ptr<GstCapabilities> m_sut;
};

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_NoDecoders)
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(nullptr));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_TRUE(m_sut->getSupportedMimeTypes(MediaSourceType::AUDIO).empty());

    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OnlyOneDecoderWithNoPads)
{
    char dummy = 0;
    GstElementFactory *decoderFactory =
        reinterpret_cast<GstElementFactory *>(&dummy); // just dummy address is needed, will not be dereferenced

    GList *listDecoders = g_list_append(nullptr, decoderFactory);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders)).Times(1);
    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_TRUE(m_sut->getSupportedMimeTypes(MediaSourceType::AUDIO).empty());

    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
    g_list_free(listDecoders);
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OnlyOneDecoderWithTwoSinkPadsAndOneSrcPad)
{
    char dummy = 0;
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummy);
    GstStaticPadTemplate decoderPadTemplate1;
    decoderPadTemplate1.direction = GST_PAD_SINK;
    GstStaticPadTemplate decoderPadTemplate2;
    decoderPadTemplate2.direction = GST_PAD_SINK;
    GstStaticPadTemplate decoderPadTemplate3;
    decoderPadTemplate3.direction = GST_PAD_SRC;

    GstCaps padTemplateCaps1;
    GstCaps padTemplateCaps2;

    GList *listDecoders = g_list_append(nullptr, decoderFactory);
    GList *decoderPadTemplates = g_list_append(nullptr, &decoderPadTemplate1);
    decoderPadTemplates = g_list_append(decoderPadTemplates, &decoderPadTemplate2);
    decoderPadTemplates = g_list_append(decoderPadTemplates, &decoderPadTemplate3);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory))
        .WillOnce(Return(decoderPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplate1.static_caps)).WillOnce(Return(&padTemplateCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplate2.static_caps)).WillOnce(Return(&padTemplateCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&padTemplateCaps2, &padTemplateCaps1)).WillOnce(Return(false));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(nullptr));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["audio/mpeg, mpegversion=(int)4"], &padTemplateCaps1))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["audio/x-opus"], &padTemplateCaps2)).WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&padTemplateCaps1)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&padTemplateCaps2)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders)).Times(1);

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::AUDIO),
                UnorderedElementsAre("audio/mp4", "audio/aac", "audio/x-eac3", "audio/x-opus"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("audio/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/beep-boop3"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));

    g_list_free(decoderPadTemplates);
    g_list_free(listDecoders);
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OnlyOneDecoderWithTwoPadsWithTheSameCaps)
{
    char dummy = 0;
    GstElementFactory *decoderFactory =
        reinterpret_cast<GstElementFactory *>(&dummy); // just dummy address is needed, will not be dereferenced
    GstStaticPadTemplate decoderPadTemplate1;
    decoderPadTemplate1.direction = GST_PAD_SINK;
    GstStaticPadTemplate decoderPadTemplate2;
    decoderPadTemplate2.direction = GST_PAD_SINK;

    GstCaps padTemplateCaps1;
    GstCaps padTemplateCaps2;

    GList *listDecoders = g_list_append(nullptr, decoderFactory);
    GList *decoderPadTemplates = g_list_append(nullptr, &decoderPadTemplate1);
    decoderPadTemplates = g_list_append(decoderPadTemplates, &decoderPadTemplate2);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory))
        .WillOnce(Return(decoderPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplate1.static_caps)).WillOnce(Return(&padTemplateCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplate2.static_caps)).WillOnce(Return(&padTemplateCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&padTemplateCaps2, &padTemplateCaps1)).WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(nullptr));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["audio/mpeg, mpegversion=(int)4"], &padTemplateCaps1))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&padTemplateCaps1)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&padTemplateCaps2)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders)).Times(1);

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::AUDIO),
                UnorderedElementsAre("audio/mp4", "audio/aac", "audio/x-eac3"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("audio/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/beep-boop3"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));

    g_list_free(decoderPadTemplates);
    g_list_free(listDecoders);
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OneDecoderWithOneSinkPad_ParserWithConnectableSrcPad)
{
    char dummyDecoder = 0;
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = 0;
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink;
    decoderPadTemplateSink.direction = GST_PAD_SINK;

    GstStaticPadTemplate parserPadTemplateSink;
    parserPadTemplateSink.direction = GST_PAD_SINK;
    GstStaticPadTemplate parserPadTemplateSrc;
    parserPadTemplateSrc.direction = GST_PAD_SRC;

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;

    GList *listDecoders = g_list_append(nullptr, decoderFactory);
    GList *listParsers = g_list_append(nullptr, parserFactory);
    GList *decoderPadTemplates = g_list_append(nullptr, &decoderPadTemplateSink);
    GList *parserPadTemplates = g_list_append(nullptr, &parserPadTemplateSink);
    parserPadTemplates = g_list_append(parserPadTemplates, &parserPadTemplateSrc);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory))
        .WillOnce(Return(decoderPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplateSink.static_caps))
        .WillOnce(Return(&decoderPadTemplateCapsSink));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(listParsers));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(parserFactory))
        .WillOnce(Return(parserPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSrc.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSink.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSink));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h265"], &parserPadTemplateCapsSink))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&decoderPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSrc)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listParsers)).Times(1);

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), UnorderedElementsAre("video/h264", "video/h265"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/x-av1"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));

    g_list_free(decoderPadTemplates);
    g_list_free(listDecoders);
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OneDecoderWithOneSinkPad_ParserWithNoConnectableSrcPad)
{
    char dummyDecoder = 0;
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = 0;
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink;
    decoderPadTemplateSink.direction = GST_PAD_SINK;

    GstStaticPadTemplate parserPadTemplateSink;
    parserPadTemplateSink.direction = GST_PAD_SINK;
    GstStaticPadTemplate parserPadTemplateSrc;
    parserPadTemplateSrc.direction = GST_PAD_SRC;

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;

    GList *listDecoders = g_list_append(nullptr, decoderFactory);
    GList *listParsers = g_list_append(nullptr, parserFactory);
    GList *decoderPadTemplates = g_list_append(nullptr, &decoderPadTemplateSink);
    GList *parserPadTemplates = g_list_append(nullptr, &parserPadTemplateSink);
    parserPadTemplates = g_list_append(parserPadTemplates, &parserPadTemplateSrc);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory))
        .WillOnce(Return(decoderPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplateSink.static_caps))
        .WillOnce(Return(&decoderPadTemplateCapsSink));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(listParsers));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(parserFactory))
        .WillOnce(Return(parserPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSrc.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(false));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&decoderPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSrc)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listParsers)).Times(1);

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), UnorderedElementsAre("video/h264"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h265"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));

    g_list_free(decoderPadTemplates);
    g_list_free(listDecoders);
}

TEST_F(GstCapabilitiesTest,
       CreateGstCapabilities_OneDecoderWithOneSinkPad_ParserWithConnectableSrcPadButNotRialtoMimeTypes)
{
    char dummyDecoder = 0;
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = 0;
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink;
    decoderPadTemplateSink.direction = GST_PAD_SINK;

    GstStaticPadTemplate parserPadTemplateSink;
    parserPadTemplateSink.direction = GST_PAD_SINK;
    GstStaticPadTemplate parserPadTemplateSrc;
    parserPadTemplateSrc.direction = GST_PAD_SRC;

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;

    GList *listDecoders = g_list_append(nullptr, decoderFactory);
    GList *listParsers = g_list_append(nullptr, parserFactory);
    GList *decoderPadTemplates = g_list_append(nullptr, &decoderPadTemplateSink);
    GList *parserPadTemplates = g_list_append(nullptr, &parserPadTemplateSink);
    parserPadTemplates = g_list_append(parserPadTemplates, &parserPadTemplateSrc);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory))
        .WillOnce(Return(decoderPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplateSink.static_caps))
        .WillOnce(Return(&decoderPadTemplateCapsSink));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(listParsers));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(parserFactory))
        .WillOnce(Return(parserPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSrc.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSink.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSink));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h265"], &parserPadTemplateCapsSink))
        .WillOnce(Return(false));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&decoderPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSrc)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listParsers)).Times(1);

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_TRUE(m_sut->getSupportedMimeTypes(MediaSourceType::AUDIO).empty());

    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));

    g_list_free(decoderPadTemplates);
    g_list_free(listDecoders);
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_TwoDecodersWithOneSinkPad_ParserWithMatchingSrcPad)
{
    char dummyDecoder = {};
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);
    char dummyDecoder2 = {};
    GstElementFactory *decoderFactory2 = reinterpret_cast<GstElementFactory *>(&dummyDecoder2);

    char dummyParser = {};
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink;
    decoderPadTemplateSink.direction = GST_PAD_SINK;
    GstStaticPadTemplate decoderPadTemplateSink2;
    decoderPadTemplateSink2.direction = GST_PAD_SINK;

    GstStaticPadTemplate parserPadTemplateSink;
    parserPadTemplateSink.direction = GST_PAD_SINK;
    GstStaticPadTemplate parserPadTemplateSrc;
    parserPadTemplateSrc.direction = GST_PAD_SRC;

    GstCaps decoderPadTemplateCapsSink;
    GstCaps decoderPadTemplateCapsSink2;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;

    GList *listDecoders = g_list_append(nullptr, decoderFactory);
    listDecoders = g_list_append(listDecoders, decoderFactory2);
    GList *listParsers = g_list_append(nullptr, parserFactory);
    GList *decoderPadTemplates = g_list_append(nullptr, &decoderPadTemplateSink);
    GList *decoderPadTemplates2 = g_list_append(nullptr, &decoderPadTemplateSink2);
    GList *parserPadTemplates = g_list_append(nullptr, &parserPadTemplateSink);
    parserPadTemplates = g_list_append(parserPadTemplates, &parserPadTemplateSrc);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory))
        .WillOnce(Return(decoderPadTemplates));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory2))
        .WillOnce(Return(decoderPadTemplates2));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplateSink.static_caps))
        .WillOnce(Return(&decoderPadTemplateCapsSink));
    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplateSink2.static_caps))
        .WillOnce(Return(&decoderPadTemplateCapsSink2));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&decoderPadTemplateCapsSink2, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(listParsers));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(parserFactory))
        .Times(2)
        .WillRepeatedly(Return(parserPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSrc.static_caps))
        .Times(2)
        .WillRepeatedly(Return(&parserPadTemplateCapsSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink2, &parserPadTemplateCapsSrc))
        .WillOnce(Return(false));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSink.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSink));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink2))
        .WillOnce(Return(true));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h265"], &decoderPadTemplateCapsSink2))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&decoderPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&decoderPadTemplateCapsSink2)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSrc))
        .Times(2); // it's also created with gstStaticCapsGet twice
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listParsers)).Times(1);

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), UnorderedElementsAre("video/h264", "video/h265"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/x-av1"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));

    g_list_free(decoderPadTemplates);
    g_list_free(listDecoders);
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OneDecodersWithOneSinkPad_ParserWithTwoSrcPadsAndSecondConnectable)
{
    char dummyDecoder = {};
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = {};
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink;
    decoderPadTemplateSink.direction = GST_PAD_SINK;

    GstStaticPadTemplate parserPadTemplateSink;
    parserPadTemplateSink.direction = GST_PAD_SINK;
    GstStaticPadTemplate parserPadTemplateSrc;
    parserPadTemplateSrc.direction = GST_PAD_SRC;
    GstStaticPadTemplate parserPadTemplateSrc2;
    parserPadTemplateSrc2.direction = GST_PAD_SRC;

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;
    GstCaps parserPadTemplateCapsSrc2;

    GList *listDecoders = g_list_append(nullptr, decoderFactory);
    GList *listParsers = g_list_append(nullptr, parserFactory);

    GList *decoderPadTemplates = g_list_append(nullptr, &decoderPadTemplateSink);
    GList *parserPadTemplates = g_list_append(nullptr, &parserPadTemplateSink);
    parserPadTemplates = g_list_append(parserPadTemplates, &parserPadTemplateSrc);
    parserPadTemplates = g_list_append(parserPadTemplates, &parserPadTemplateSrc2);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory))
        .WillOnce(Return(decoderPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplateSink.static_caps))
        .WillOnce(Return(&decoderPadTemplateCapsSink));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(listParsers));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(parserFactory))
        .WillOnce(Return(parserPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSrc.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSrc2.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSrc2));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc2))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSink.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSink));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h265"], &parserPadTemplateCapsSink))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&decoderPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSrc)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSrc2)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listParsers)).Times(1);

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), UnorderedElementsAre("video/h264", "video/h265"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/x-av1"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));

    g_list_free(decoderPadTemplates);
    g_list_free(listDecoders);
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OneDecodersWithOneSinkPads_TwoParsersWithConnectableSrcPads)
{
    char dummyDecoder = {};
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = {};
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);
    char dummyParser2 = {};
    GstElementFactory *parserFactory2 = reinterpret_cast<GstElementFactory *>(&dummyParser2);

    GstStaticPadTemplate decoderPadTemplateSink;
    decoderPadTemplateSink.direction = GST_PAD_SINK;

    GstStaticPadTemplate parserPadTemplateSink;
    parserPadTemplateSink.direction = GST_PAD_SINK;
    GstStaticPadTemplate parserPadTemplateSrc;
    parserPadTemplateSrc.direction = GST_PAD_SRC;
    GstStaticPadTemplate parserPadTemplateSink2;
    parserPadTemplateSink2.direction = GST_PAD_SINK;
    GstStaticPadTemplate parserPadTemplateSrc2;
    parserPadTemplateSrc2.direction = GST_PAD_SRC;

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;
    GstCaps parserPadTemplateCapsSink2;
    GstCaps parserPadTemplateCapsSrc2;

    GList *listDecoders = g_list_append(nullptr, decoderFactory);
    GList *listParsers = g_list_append(nullptr, parserFactory);
    listParsers = g_list_append(listParsers, parserFactory2);

    GList *decoderPadTemplates = g_list_append(nullptr, &decoderPadTemplateSink);
    GList *parserPadTemplates = g_list_append(nullptr, &parserPadTemplateSink);
    parserPadTemplates = g_list_append(parserPadTemplates, &parserPadTemplateSrc);
    GList *parserPadTemplates2 = g_list_append(nullptr, &parserPadTemplateSink2);
    parserPadTemplates2 = g_list_append(parserPadTemplates2, &parserPadTemplateSrc2);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(listDecoders));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory))
        .WillOnce(Return(decoderPadTemplates));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&decoderPadTemplateSink.static_caps))
        .WillOnce(Return(&decoderPadTemplateCapsSink));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(listParsers));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(parserFactory))
        .WillOnce(Return(parserPadTemplates));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(parserFactory2))
        .WillOnce(Return(parserPadTemplates2));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSrc.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSrc2.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSrc2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc2))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSink.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSink));
    EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&parserPadTemplateSink2.static_caps))
        .WillOnce(Return(&parserPadTemplateCapsSink2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink2, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink2, &parserPadTemplateCapsSink))
        .WillOnce(Return(false));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-h265"], &parserPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsSubset(&m_capsMap["video/x-av1"], &parserPadTemplateCapsSink2))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&decoderPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSink)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSrc)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSink2)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&parserPadTemplateCapsSrc2)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listDecoders)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(listParsers)).Times(1);

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO),
                UnorderedElementsAre("video/h264", "video/h265", "video/x-av1"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/x-av1"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/x-vp9"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));

    g_list_free(decoderPadTemplates);
    g_list_free(listDecoders);
}