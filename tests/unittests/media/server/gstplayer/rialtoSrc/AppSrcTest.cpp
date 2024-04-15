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

#include "DecryptionServiceMock.h"
#include "GlibWrapperFactoryMock.h"
#include "GlibWrapperMock.h"
#include "GstDecryptorElementFactoryMock.h"
#include "GstSrc.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include "IFactoryAccessor.h"
#include "Matchers.h"
#include "MediaSourceUtil.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::_;
using ::testing::HasSubstr;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

class RialtoServerAppSrcGstSrcTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock;
    std::shared_ptr<StrictMock<GlibWrapperFactoryMock>> m_glibWrapperFactoryMock;
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock;
    std::shared_ptr<StrictMock<GstDecryptorElementFactoryMock>> m_decryptorFactoryMock;
    std::shared_ptr<StrictMock<DecryptionServiceMock>> m_decryptionServiceMock;
    std::unique_ptr<IGstSrc> m_gstSrc;

    GstElement m_appsrc = {};
    GstElement m_decryptor = {};
    GstElement m_payloader = {};
    GstElement m_queue = {};
    GstObject m_factory = {};
    GstBin m_rialtoSrcBin = {};
    GstRialtoSrcPrivate m_rialtoSrcPriv = {};
    GstRialtoSrc m_rialtoSrc = {m_rialtoSrcBin, &m_rialtoSrcPriv};
    GstAppSrcCallbacks m_callbacks = {};
    GstPad m_target = {};
    GstPad m_pad = {};
    gchar *m_name = "src_0";
    gchar *m_audioDecryptorName = "rialtodecryptoraudio_0";
    gchar *m_videoDecryptorName = "rialtodecryptorvideo_0";
    StreamInfo m_streamInfo;
    GstCaps m_dummyCaps;
    GstCaps m_dummyCaps2;
    GstStructure m_dummyStructure;

    RialtoServerAppSrcGstSrcTest() : m_streamInfo{&m_appsrc, true} {}

    void SetUp() override
    {
        m_gstWrapperFactoryMock = std::make_shared<StrictMock<GstWrapperFactoryMock>>();
        m_gstWrapperMock = std::make_shared<StrictMock<GstWrapperMock>>();

        m_glibWrapperFactoryMock = std::make_shared<StrictMock<GlibWrapperFactoryMock>>();
        m_glibWrapperMock = std::make_shared<StrictMock<GlibWrapperMock>>();

        m_decryptorFactoryMock = std::make_shared<StrictMock<GstDecryptorElementFactoryMock>>();
        m_decryptionServiceMock = std::make_shared<StrictMock<DecryptionServiceMock>>();

        IFactoryAccessor::instance().glibWrapperFactory() = m_glibWrapperFactoryMock;
        IFactoryAccessor::instance().gstWrapperFactory() = m_gstWrapperFactoryMock;

        createGstSrc();
    }

    void TearDown() override
    {
        IFactoryAccessor::instance().glibWrapperFactory() = nullptr;
        IFactoryAccessor::instance().gstWrapperFactory() = nullptr;
    }

    void createGstSrc()
    {
        EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillRepeatedly(Return(m_gstWrapperMock));
        EXPECT_CALL(*m_glibWrapperFactoryMock, getGlibWrapper()).WillRepeatedly(Return(m_glibWrapperMock));

        EXPECT_NO_THROW(m_gstSrc = std::make_unique<GstSrc>(m_gstWrapperFactoryMock, m_glibWrapperFactoryMock,
                                                            m_decryptorFactoryMock));
        EXPECT_NE(m_gstSrc, nullptr);
    }

    void expectSettings(guint64 max)
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_streamInfo.appSrc, StrEq("block")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_streamInfo.appSrc, StrEq("format")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_streamInfo.appSrc, StrEq("stream-type")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_streamInfo.appSrc, StrEq("min-percent")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_streamInfo.appSrc, StrEq("handle-segment-change")));

        EXPECT_CALL(*m_gstWrapperMock,
                    gstAppSrcSetCallbacks(GST_APP_SRC(m_streamInfo.appSrc), &m_callbacks, this, nullptr));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(GST_APP_SRC(m_streamInfo.appSrc), max));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstAppSrcSetStreamType(GST_APP_SRC(m_streamInfo.appSrc), GST_APP_STREAM_TYPE_SEEKABLE));
    }

    void expectBin(GstElement *expectedElement)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_rialtoSrc), expectedElement));
    }

    void expectSetupPad(GstElement *expectedSrcElement)
    {
        EXPECT_CALL(*m_glibWrapperMock, gStrdupPrintfStub(HasSubstr("src"))).WillOnce(Return(m_name));

        EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(expectedSrcElement, StrEq("src"))).WillOnce(Return(&m_target));
        EXPECT_CALL(*m_gstWrapperMock, gstGhostPadNew(StrEq(m_name), &m_target)).WillOnce(Return(&m_pad));
        EXPECT_CALL(*m_gstWrapperMock, gstPadSetQueryFunction(&m_pad, NotNullMatcher()));
        EXPECT_CALL(*m_gstWrapperMock, gstPadSetActive(&m_pad, TRUE));
        EXPECT_CALL(*m_gstWrapperMock, gstElementAddPad(GST_ELEMENT(&m_rialtoSrc), &m_pad));

        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_target));
        EXPECT_CALL(*m_glibWrapperMock, gFree(PtrStrMatcher(m_name)));
    }

    void expectSyncElement(GstElement *expectedElement)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSyncStateWithParent(expectedElement));
    }

    void expectLinkDecryptor(GstElement *expectedSrcElement, gchar *decryptorName)
    {
        expectCreateDecryptor(decryptorName);
        expectBin(&m_decryptor);
        expectSyncElement(&m_decryptor);
        EXPECT_CALL(*m_gstWrapperMock, gstElementLink(expectedSrcElement, &m_decryptor));
    }

    void expectLinkPayloader(GstElement *expectedSrcElement)
    {
        EXPECT_CALL(*m_glibWrapperMock, gOnceInitEnter(_)).WillOnce(Return(TRUE));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("svppay")))
            .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_factory)));
        EXPECT_CALL(*m_glibWrapperMock, gOnceInitLeave(_, 1));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_factory), _))
            .WillOnce(Return(&m_payloader));

        expectBin(&m_payloader);
        expectSyncElement(&m_payloader);
        EXPECT_CALL(*m_gstWrapperMock, gstElementLink(expectedSrcElement, &m_payloader));
    }

    void expectAddDefaultStreamFormat()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(m_streamInfo.appSrc))).WillOnce(Return(&m_dummyCaps));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsGetStructure(&m_dummyCaps, 0)).WillOnce(Return(&m_dummyStructure));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureHasName(&m_dummyStructure, StrEq("video/x-h264"))).WillOnce(Return(true));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&m_dummyCaps)).WillOnce(Return(&m_dummyCaps2));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureHasField(&m_dummyStructure, StrEq("stream-format")))
            .WillOnce(Return(false));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureHasField(&m_dummyStructure, StrEq("codec_data"))).WillOnce(Return(false));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleStringStub(&m_dummyCaps2, StrEq("stream-format"), G_TYPE_STRING,
                                                                  StrEq("byte-stream")));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(m_streamInfo.appSrc), &m_dummyCaps2));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_dummyCaps2));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_dummyCaps));
    }

    void expectLinkQueue(GstElement *expectedSrcElement)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("queue"), _)).WillOnce(Return(&m_queue));

        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_queue, StrEq("max-size-buffers")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_queue, StrEq("max-size-bytes")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_queue, StrEq("max-size-time")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_queue, StrEq("silent")));

        expectBin(&m_queue);
        expectSyncElement(&m_queue);
        EXPECT_CALL(*m_gstWrapperMock, gstElementLink(expectedSrcElement, &m_queue));
    }

    void expectCreateDecryptor(gchar *decryptorName)
    {
        EXPECT_CALL(*m_decryptorFactoryMock,
                    createDecryptorElement(StrEq(decryptorName),
                                           reinterpret_cast<IDecryptionService *>(m_decryptionServiceMock.get()), _))
            .WillOnce(Return(&m_decryptor));
        EXPECT_CALL(*m_glibWrapperMock, gStrdupPrintfStub(StrEq(decryptorName))).WillOnce(Return(decryptorName));
        EXPECT_CALL(*m_glibWrapperMock, gFree(PtrStrMatcher(decryptorName)));
    }
};

/**
 * Test that GstSrc can add and setup a video source
 */
TEST_F(RialtoServerAppSrcGstSrcTest, SetupVideo)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(m_streamInfo.appSrc))).WillOnce(Return(&m_dummyCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsGetStructure(&m_dummyCaps, 0)).WillOnce(Return(&m_dummyStructure));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureHasName(&m_dummyStructure, StrEq("video/x-h264"))).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureHasName(&m_dummyStructure, StrEq("video/x-h265"))).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_dummyCaps));

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.appSrc);
    expectSyncElement(m_streamInfo.appSrc);
    expectLinkDecryptor(m_streamInfo.appSrc, m_videoDecryptorName);
    expectLinkPayloader(&m_decryptor);
    expectLinkQueue(&m_payloader);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppSrc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc can add and setup a video source with no stream-format set.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, SetupVideoH264WithoutStreamFormat)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.appSrc);
    expectSyncElement(m_streamInfo.appSrc);
    expectLinkDecryptor(m_streamInfo.appSrc, m_videoDecryptorName);
    expectLinkPayloader(&m_decryptor);
    expectAddDefaultStreamFormat();
    expectLinkQueue(&m_payloader);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppSrc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc can add and setup a video source with stream-format set.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, SetupVideoWithStreamFormat)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(m_streamInfo.appSrc))).WillOnce(Return(&m_dummyCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsGetStructure(&m_dummyCaps, 0)).WillOnce(Return(&m_dummyStructure));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureHasName(&m_dummyStructure, StrEq("video/x-h264"))).WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureHasField(&m_dummyStructure, StrEq("stream-format"))).WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureHasField(&m_dummyStructure, StrEq("codec_data"))).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_dummyCaps));

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.appSrc);
    expectSyncElement(m_streamInfo.appSrc);
    expectLinkDecryptor(m_streamInfo.appSrc, m_videoDecryptorName);
    expectLinkPayloader(&m_decryptor);
    expectLinkQueue(&m_payloader);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppSrc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc can add and setup a video source with codec data set.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, SetupVideoWithCodecData)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(m_streamInfo.appSrc))).WillOnce(Return(&m_dummyCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsGetStructure(&m_dummyCaps, 0)).WillOnce(Return(&m_dummyStructure));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureHasName(&m_dummyStructure, StrEq("video/x-h264"))).WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureHasField(&m_dummyStructure, StrEq("stream-format"))).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureHasField(&m_dummyStructure, StrEq("codec_data"))).WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_dummyCaps));

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.appSrc);
    expectSyncElement(m_streamInfo.appSrc);
    expectLinkDecryptor(m_streamInfo.appSrc, m_videoDecryptorName);
    expectLinkPayloader(&m_decryptor);
    expectLinkQueue(&m_payloader);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppSrc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test the factory
 */
TEST_F(RialtoServerAppSrcGstSrcTest, FactoryCreatesObject)
{
    std::shared_ptr<firebolt::rialto::server::IGstSrcFactory> factory =
        firebolt::rialto::server::IGstSrcFactory::getFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_NE(factory->getGstSrc(), nullptr);
}

/**
 * Test that GstSrc can add and setup a audio source.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, SetupAudio)
{
    guint64 audioMaxBytes = 512 * 1024;

    expectSettings(audioMaxBytes);
    expectBin(m_streamInfo.appSrc);
    expectSyncElement(m_streamInfo.appSrc);
    expectLinkDecryptor(m_streamInfo.appSrc, m_audioDecryptorName);
    expectLinkQueue(&m_decryptor);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppSrc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::AUDIO);
}

/**
 * Test that GstSrc still created if decryptor element fails.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, DecryptorFailure)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    EXPECT_CALL(*m_glibWrapperMock, gStrdupPrintfStub(StrEq(m_videoDecryptorName))).WillOnce(Return(m_videoDecryptorName));
    EXPECT_CALL(*m_decryptorFactoryMock,
                createDecryptorElement(StrEq(m_videoDecryptorName),
                                       reinterpret_cast<IDecryptionService *>(m_decryptionServiceMock.get()), _))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapperMock, gFree(PtrStrMatcher(m_videoDecryptorName)));

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.appSrc);
    expectSyncElement(m_streamInfo.appSrc);
    expectLinkPayloader(m_streamInfo.appSrc);
    expectAddDefaultStreamFormat();
    expectLinkQueue(&m_payloader);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppSrc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc still created if payloader element fails.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, PayloaderFailure)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    EXPECT_CALL(*m_glibWrapperMock, gOnceInitEnter(_)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("svppay")))
        .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_factory)));
    EXPECT_CALL(*m_glibWrapperMock, gOnceInitLeave(_, 1));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_factory), _))
        .WillOnce(Return(nullptr));

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.appSrc);
    expectSyncElement(m_streamInfo.appSrc);
    expectLinkDecryptor(m_streamInfo.appSrc, m_videoDecryptorName);
    expectLinkQueue(&m_decryptor);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppSrc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc still created if queue element fails.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, QueueFailure)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("queue"), _)).WillOnce(Return(nullptr));

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.appSrc);
    expectSyncElement(m_streamInfo.appSrc);
    expectLinkDecryptor(m_streamInfo.appSrc, m_videoDecryptorName);
    expectLinkPayloader(&m_decryptor);
    expectAddDefaultStreamFormat();
    expectSetupPad(&m_payloader);

    m_gstSrc->setupAndAddAppSrc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc can add and setup a video source.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, NotDrm)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;
    m_streamInfo.hasDrm = false;

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.appSrc);
    expectSyncElement(m_streamInfo.appSrc);
    expectSetupPad(m_streamInfo.appSrc);

    m_gstSrc->setupAndAddAppSrc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc can signal all app sources are added.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, AllSrcsAdded)
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementNoMorePads(GST_ELEMENT(&m_rialtoSrc)));

    m_gstSrc->allAppSrcsAdded(GST_ELEMENT(&m_rialtoSrc));
}
