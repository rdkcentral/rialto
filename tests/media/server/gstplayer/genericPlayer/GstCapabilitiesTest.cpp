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

template <typename T> class GListWrapper
{
public:
    explicit GListWrapper(std::initializer_list<T> elements)
    {
        for (T element : elements)
        {
            m_list = g_list_append(m_list, element);
        }
    }
    ~GListWrapper() { g_list_free(m_list); }
    GList *get() { return m_list; }

private:
    GList *m_list = nullptr;
};

class GstCapabilitiesTest : public testing::Test
{
public:
    GstCapabilitiesTest()
        // valid values of GstCaps are not needed, only addresses will be used
        : m_capsMap{{"audio/mpeg, mpegversion=(int)4", {}},
                    {"audio/x-eac3", {}},
                    {"audio/x-opus", {}},
                    {"video/x-av1", {}},
                    {"video/x-h264", {}},
                    {"video/x-h265", {}},
                    {"video/x-vp9", {}},
                    {"video/mpeg, mpegversion=(int)4", {}},
                    {"video/x-h264(memory:DMABuf)", {}},
                    {"video/x-h265(memory:DMABuf)", {}},
                    {"video/x-av1(memory:DMABuf)", {}},
                    {"video/x-vp9(memory:DMABuf)", {}}}
    {
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

    void expectGetFactoryListAndFreeList(GList *list, GstElementFactoryListType type, GstRank minrank)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(type, minrank)).WillOnce(Return(list));
        EXPECT_CALL(*m_gstWrapperMock, gstPluginFeatureListFree(list)).Times(1);
    }

    void expectGetStaticPadTemplates(GstElementFactory *factory, GList *list, int count = 1)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(factory))
            .Times(count)
            .WillRepeatedly(Return(list));
    }

    void expectGetStaticCapsAndCapsUnref(GstStaticPadTemplate &padTemplate, GstCaps *caps, int count = 1)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstStaticCapsGet(&padTemplate.static_caps)).Times(count).WillRepeatedly(Return(caps));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(caps)).Times(count);
    }

    GstStaticPadTemplate createSinkPadTemplate()
    {
        GstStaticPadTemplate decoderPadTemplate;
        decoderPadTemplate.direction = GST_PAD_SINK;
        return decoderPadTemplate;
    }

    GstStaticPadTemplate createSrcPadTemplate()
    {
        GstStaticPadTemplate decoderPadTemplate;
        decoderPadTemplate.direction = GST_PAD_SRC;
        return decoderPadTemplate;
    }

    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
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

/**
 * Test the factory
 */
TEST_F(GstCapabilitiesTest, FactoryCreatesObject)
{
    std::shared_ptr<firebolt::rialto::server::IGstCapabilitiesFactory> factory =
      firebolt::rialto::server::IGstCapabilitiesFactory::getFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_NE(factory->createGstCapabilities(), nullptr);
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OnlyOneDecoderWithNoPads)
{
    char dummy = 0;
    GstElementFactory *decoderFactory =
        reinterpret_cast<GstElementFactory *>(&dummy); // just dummy address is needed, will not be dereferenced

    GListWrapper<GstElementFactory *> listDecoders({decoderFactory});

    expectGetFactoryListAndFreeList(listDecoders.get(), GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryGetStaticPadTemplates(decoderFactory)).WillOnce(Return(nullptr));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_TRUE(m_sut->getSupportedMimeTypes(MediaSourceType::AUDIO).empty());

    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OnlyOneDecoderWithTwoSinkPadsAndOneSrcPad)
{
    char dummy = 0;
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummy);
    GstStaticPadTemplate decoderPadTemplate1 = createSinkPadTemplate();
    GstStaticPadTemplate decoderPadTemplate2 = createSinkPadTemplate();
    GstStaticPadTemplate decoderPadTemplate3 = createSrcPadTemplate();

    GstCaps padTemplateCaps1;
    GstCaps padTemplateCaps2;

    GListWrapper<GstElementFactory *> listDecoders{decoderFactory};
    GListWrapper<GstStaticPadTemplate *> decoderPadTemplates{&decoderPadTemplate1, &decoderPadTemplate2,
                                                             &decoderPadTemplate3};

    expectGetFactoryListAndFreeList(listDecoders.get(), GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(decoderFactory, decoderPadTemplates.get());

    expectGetStaticCapsAndCapsUnref(decoderPadTemplate1, &padTemplateCaps1);
    expectGetStaticCapsAndCapsUnref(decoderPadTemplate2, &padTemplateCaps2);

    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&padTemplateCaps2, &padTemplateCaps1)).WillOnce(Return(false));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(nullptr));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["audio/mpeg, mpegversion=(int)4"], &padTemplateCaps1))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["audio/x-opus"], &padTemplateCaps2)).WillOnce(Return(true));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::AUDIO),
                UnorderedElementsAre("audio/mp4", "audio/aac", "audio/x-eac3", "audio/x-opus"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("audio/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/beep-boop3"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OnlyOneDecoderWithTwoPadsWithTheSameCaps)
{
    char dummy = 0;
    GstElementFactory *decoderFactory =
        reinterpret_cast<GstElementFactory *>(&dummy); // just dummy address is needed, will not be dereferenced
    GstStaticPadTemplate decoderPadTemplate1 = createSinkPadTemplate();
    GstStaticPadTemplate decoderPadTemplate2 = createSinkPadTemplate();

    GstCaps padTemplateCaps1;
    GstCaps padTemplateCaps2;

    GListWrapper<GstElementFactory *> listDecoders{decoderFactory};
    GListWrapper<GstStaticPadTemplate *> decoderPadTemplates{&decoderPadTemplate1, &decoderPadTemplate2};

    expectGetFactoryListAndFreeList(listDecoders.get(), GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(decoderFactory, decoderPadTemplates.get());

    expectGetStaticCapsAndCapsUnref(decoderPadTemplate1, &padTemplateCaps1);
    expectGetStaticCapsAndCapsUnref(decoderPadTemplate2, &padTemplateCaps2);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&padTemplateCaps2, &padTemplateCaps1)).WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL))
        .WillOnce(Return(nullptr));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["audio/mpeg, mpegversion=(int)4"], &padTemplateCaps1))
        .WillOnce(Return(true));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::AUDIO),
                UnorderedElementsAre("audio/mp4", "audio/aac", "audio/x-eac3"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("audio/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/beep-boop3"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OneDecoderWithOneSinkPad_ParserWithConnectableSrcPad)
{
    char dummyDecoder = 0;
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = 0;
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSrc = createSrcPadTemplate();

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;

    GListWrapper<GstElementFactory *> listDecoders{decoderFactory};
    GListWrapper<GstElementFactory *> listParsers{parserFactory};

    GListWrapper<GstStaticPadTemplate *> decoderPadTemplates{&decoderPadTemplateSink};
    GListWrapper<GstStaticPadTemplate *> parserPadTemplates{&parserPadTemplateSink, &parserPadTemplateSrc};

    expectGetFactoryListAndFreeList(listDecoders.get(), GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(decoderFactory, decoderPadTemplates.get());

    expectGetStaticCapsAndCapsUnref(decoderPadTemplateSink, &decoderPadTemplateCapsSink);

    expectGetFactoryListAndFreeList(listParsers.get(), GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(parserFactory, parserPadTemplates.get());
    expectGetStaticCapsAndCapsUnref(parserPadTemplateSrc, &parserPadTemplateCapsSrc);

    expectGetStaticCapsAndCapsUnref(parserPadTemplateSink, &parserPadTemplateCapsSink);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h265"], &parserPadTemplateCapsSink))
        .WillOnce(Return(true));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), UnorderedElementsAre("video/h264", "video/h265"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/x-av1"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OneDecoderWithOneSinkPad_ParserWithNoConnectableSrcPad)
{
    char dummyDecoder = 0;
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = 0;
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSrc = createSrcPadTemplate();

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;

    GListWrapper<GstElementFactory *> listDecoders{decoderFactory};
    GListWrapper<GstElementFactory *> listParsers{parserFactory};

    GListWrapper<GstStaticPadTemplate *> decoderPadTemplates{&decoderPadTemplateSink};
    GListWrapper<GstStaticPadTemplate *> parserPadTemplates{&parserPadTemplateSink, &parserPadTemplateSrc};

    expectGetFactoryListAndFreeList(listDecoders.get(), GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(decoderFactory, decoderPadTemplates.get());
    expectGetStaticCapsAndCapsUnref(decoderPadTemplateSink, &decoderPadTemplateCapsSink);

    expectGetFactoryListAndFreeList(listParsers.get(), GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(parserFactory, parserPadTemplates.get());
    expectGetStaticCapsAndCapsUnref(parserPadTemplateSrc, &parserPadTemplateCapsSrc);

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsCanIntersect(&m_capsMap["video/mpeg, mpegversion=(int)4"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), UnorderedElementsAre("video/mp4"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h265"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));
}

TEST_F(GstCapabilitiesTest,
       CreateGstCapabilities_OneDecoderWithOneSinkPad_ParserWithConnectableSrcPadButNotRialtoMimeTypes)
{
    char dummyDecoder = 0;
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = 0;
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSrc = createSrcPadTemplate();

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;

    GListWrapper<GstElementFactory *> listDecoders{decoderFactory};
    GListWrapper<GstElementFactory *> listParsers{parserFactory};

    GListWrapper<GstStaticPadTemplate *> decoderPadTemplates{&decoderPadTemplateSink};
    GListWrapper<GstStaticPadTemplate *> parserPadTemplates{&parserPadTemplateSink, &parserPadTemplateSrc};

    expectGetFactoryListAndFreeList(listDecoders.get(), GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(decoderFactory, decoderPadTemplates.get());

    expectGetStaticCapsAndCapsUnref(decoderPadTemplateSink, &decoderPadTemplateCapsSink);

    expectGetFactoryListAndFreeList(listParsers.get(), GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(parserFactory, parserPadTemplates.get());
    expectGetStaticCapsAndCapsUnref(parserPadTemplateSrc, &parserPadTemplateCapsSrc);

    expectGetStaticCapsAndCapsUnref(parserPadTemplateSink, &parserPadTemplateCapsSink);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h265"], &parserPadTemplateCapsSink))
        .WillOnce(Return(false));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_TRUE(m_sut->getSupportedMimeTypes(MediaSourceType::AUDIO).empty());

    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_TwoDecodersWithOneSinkPad_ParserWithMatchingSrcPad)
{
    char dummyDecoder = {};
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);
    char dummyDecoder2 = {};
    GstElementFactory *decoderFactory2 = reinterpret_cast<GstElementFactory *>(&dummyDecoder2);

    char dummyParser = {};
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate decoderPadTemplateSink2 = createSinkPadTemplate();

    GstStaticPadTemplate parserPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSrc = createSrcPadTemplate();

    GstCaps decoderPadTemplateCapsSink;
    GstCaps decoderPadTemplateCapsSink2;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;

    GListWrapper<GstElementFactory *> listDecoders{decoderFactory, decoderFactory2};
    GListWrapper<GstElementFactory *> listParsers{parserFactory};

    GListWrapper<GstStaticPadTemplate *> decoderPadTemplates{&decoderPadTemplateSink};
    GListWrapper<GstStaticPadTemplate *> decoderPadTemplates2{&decoderPadTemplateSink2};
    GListWrapper<GstStaticPadTemplate *> parserPadTemplates{&parserPadTemplateSink, &parserPadTemplateSrc};

    expectGetFactoryListAndFreeList(listDecoders.get(), GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(decoderFactory, decoderPadTemplates.get());
    expectGetStaticPadTemplates(decoderFactory2, decoderPadTemplates2.get());

    expectGetStaticCapsAndCapsUnref(decoderPadTemplateSink, &decoderPadTemplateCapsSink);
    expectGetStaticCapsAndCapsUnref(decoderPadTemplateSink2, &decoderPadTemplateCapsSink2);

    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&decoderPadTemplateCapsSink2, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));

    expectGetFactoryListAndFreeList(listParsers.get(), GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(parserFactory, parserPadTemplates.get(), 2);
    expectGetStaticCapsAndCapsUnref(parserPadTemplateSrc, &parserPadTemplateCapsSrc, 2);

    expectGetStaticCapsAndCapsUnref(parserPadTemplateSink, &parserPadTemplateCapsSink);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink2))
        .WillOnce(Return(true));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink2, &parserPadTemplateCapsSrc))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h265"], &decoderPadTemplateCapsSink2))
        .WillOnce(Return(true));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), UnorderedElementsAre("video/h264", "video/h265"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/x-av1"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OneDecodersWithOneSinkPad_ParserWithTwoSrcPadsAndSecondConnectable)
{
    char dummyDecoder = {};
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = {};
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);

    GstStaticPadTemplate decoderPadTemplateSink = createSinkPadTemplate();

    GstStaticPadTemplate parserPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSrc = createSrcPadTemplate();
    GstStaticPadTemplate parserPadTemplateSrc2 = createSrcPadTemplate();

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;
    GstCaps parserPadTemplateCapsSrc2;

    GListWrapper<GstElementFactory *> listDecoders{decoderFactory};
    GListWrapper<GstElementFactory *> listParsers{parserFactory};

    GListWrapper<GstStaticPadTemplate *> decoderPadTemplates{&decoderPadTemplateSink};
    GListWrapper<GstStaticPadTemplate *> parserPadTemplates{&parserPadTemplateSink, &parserPadTemplateSrc,
                                                            &parserPadTemplateSrc2};

    expectGetFactoryListAndFreeList(listDecoders.get(), GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(decoderFactory, decoderPadTemplates.get());

    expectGetStaticCapsAndCapsUnref(decoderPadTemplateSink, &decoderPadTemplateCapsSink);

    expectGetFactoryListAndFreeList(listParsers.get(), GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL);

    expectGetStaticPadTemplates(parserFactory, parserPadTemplates.get());
    expectGetStaticCapsAndCapsUnref(parserPadTemplateSrc, &parserPadTemplateCapsSrc);
    expectGetStaticCapsAndCapsUnref(parserPadTemplateSrc2, &parserPadTemplateCapsSrc2);

    expectGetStaticCapsAndCapsUnref(parserPadTemplateSink, &parserPadTemplateCapsSink);
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc2))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h265"], &parserPadTemplateCapsSink))
        .WillOnce(Return(true));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), UnorderedElementsAre("video/h264", "video/h265"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/x-av1"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));
}

TEST_F(GstCapabilitiesTest, CreateGstCapabilities_OneDecodersWithOneSinkPads_TwoParsersWithConnectableSrcPads)
{
    char dummyDecoder = {};
    GstElementFactory *decoderFactory = reinterpret_cast<GstElementFactory *>(&dummyDecoder);

    char dummyParser = {};
    GstElementFactory *parserFactory = reinterpret_cast<GstElementFactory *>(&dummyParser);
    char dummyParser2 = {};
    GstElementFactory *parserFactory2 = reinterpret_cast<GstElementFactory *>(&dummyParser2);

    GstStaticPadTemplate decoderPadTemplateSink = createSinkPadTemplate();

    GstStaticPadTemplate parserPadTemplateSink = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSrc = createSrcPadTemplate();

    GstStaticPadTemplate parserPadTemplateSink2 = createSinkPadTemplate();
    GstStaticPadTemplate parserPadTemplateSrc2 = createSrcPadTemplate();

    GstCaps decoderPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSink;
    GstCaps parserPadTemplateCapsSrc;
    GstCaps parserPadTemplateCapsSink2;
    GstCaps parserPadTemplateCapsSrc2;

    GListWrapper<GstElementFactory *> listDecoders{decoderFactory};
    GListWrapper<GstElementFactory *> listParsers{parserFactory, parserFactory2};

    GListWrapper<GstStaticPadTemplate *> decoderPadTemplates{&decoderPadTemplateSink};
    GListWrapper<GstStaticPadTemplate *> parserPadTemplates{&parserPadTemplateSink, &parserPadTemplateSrc};
    GListWrapper<GstStaticPadTemplate *> parserPadTemplates2{&parserPadTemplateSink2, &parserPadTemplateSrc2};

    expectGetFactoryListAndFreeList(listDecoders.get(), GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(decoderFactory, decoderPadTemplates.get());

    expectGetStaticCapsAndCapsUnref(decoderPadTemplateSink, &decoderPadTemplateCapsSink);

    expectGetFactoryListAndFreeList(listParsers.get(), GST_ELEMENT_FACTORY_TYPE_PARSER, GST_RANK_MARGINAL);
    expectGetStaticPadTemplates(parserFactory, parserPadTemplates.get());
    expectGetStaticPadTemplates(parserFactory2, parserPadTemplates2.get());

    expectGetStaticCapsAndCapsUnref(parserPadTemplateSrc, &parserPadTemplateCapsSrc);
    expectGetStaticCapsAndCapsUnref(parserPadTemplateSrc2, &parserPadTemplateCapsSrc2);

    expectGetStaticCapsAndCapsUnref(parserPadTemplateSink, &parserPadTemplateCapsSink);
    expectGetStaticCapsAndCapsUnref(parserPadTemplateSink2, &parserPadTemplateCapsSink2);

    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink2, &decoderPadTemplateCapsSink))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsStrictlyEqual(&parserPadTemplateCapsSink2, &parserPadTemplateCapsSink))
        .WillOnce(Return(false));

    expectCapsToMimeMapping();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&decoderPadTemplateCapsSink, &parserPadTemplateCapsSrc2))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h264"], &decoderPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-h265"], &parserPadTemplateCapsSink))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCanIntersect(&m_capsMap["video/x-av1"], &parserPadTemplateCapsSink2))
        .WillOnce(Return(true));

    m_sut = std::make_unique<GstCapabilities>(m_gstWrapperMock);

    EXPECT_THAT(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO),
                UnorderedElementsAre("video/h264", "video/h265", "video/x-av1"));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/x-av1"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/mp4"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/x-vp9"));
    EXPECT_FALSE(m_sut->isMimeTypeSupported("audio/x-opus"));
}
