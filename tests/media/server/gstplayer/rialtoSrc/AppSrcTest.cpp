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
#include "MediaSourceUtil.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

MATCHER_P(CharStrMatcher, expectedStr, "")
{
    std::string actualStr = (const char *)arg;
    return expectedStr == actualStr;
}

MATCHER(NotNullMatcher, "")
{
    return nullptr != arg;
}

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
    StreamInfo m_streamInfo = {};

    RialtoServerAppSrcGstSrcTest() : m_streamInfo{&m_appsrc, true} {}

    virtual void SetUp()
    {
        m_gstWrapperFactoryMock = std::make_shared<StrictMock<GstWrapperFactoryMock>>();
        m_gstWrapperMock = std::make_shared<StrictMock<GstWrapperMock>>();

        m_glibWrapperFactoryMock = std::make_shared<StrictMock<GlibWrapperFactoryMock>>();
        m_glibWrapperMock = std::make_shared<StrictMock<GlibWrapperMock>>();

        m_decryptorFactoryMock = std::make_shared<StrictMock<GstDecryptorElementFactoryMock>>();
        m_decryptionServiceMock = std::make_shared<StrictMock<DecryptionServiceMock>>();

        createGstSrc();
    }

    virtual void TearDown() {}

    void createGstSrc()
    {
        EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));
        EXPECT_CALL(*m_glibWrapperFactoryMock, getGlibWrapper()).WillOnce(Return(m_glibWrapperMock));

        EXPECT_NO_THROW(m_gstSrc = std::make_unique<GstSrc>(m_gstWrapperFactoryMock, m_glibWrapperFactoryMock,
                                                            m_decryptorFactoryMock););
        EXPECT_NE(m_gstSrc, nullptr);
    }

    void expectSettings(guint64 max)
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_streamInfo.m_appSrc, CharStrMatcher("block")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_streamInfo.m_appSrc, CharStrMatcher("format")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_streamInfo.m_appSrc, CharStrMatcher("stream-type")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_streamInfo.m_appSrc, CharStrMatcher("min-percent")));

        EXPECT_CALL(*m_gstWrapperMock,
                    gstAppSrcSetCallbacks(GST_APP_SRC(m_streamInfo.m_appSrc), &m_callbacks, this, nullptr));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(GST_APP_SRC(m_streamInfo.m_appSrc), max));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstAppSrcSetStreamType(GST_APP_SRC(m_streamInfo.m_appSrc), GST_APP_STREAM_TYPE_SEEKABLE));
    }

    void expectBin(GstElement *expectedElement)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_rialtoSrc), expectedElement));
    }

    void expectSetupPad(GstElement *expectedSrcElement)
    {
        EXPECT_CALL(*m_glibWrapperMock, gStrdupPrintfStub(_)).WillOnce(Return(m_name));

        EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(expectedSrcElement, CharStrMatcher("src")))
            .WillOnce(Return(&m_target));
        EXPECT_CALL(*m_gstWrapperMock, gstGhostPadNew(CharStrMatcher(m_name), &m_target)).WillOnce(Return(&m_pad));
        EXPECT_CALL(*m_gstWrapperMock, gstPadSetQueryFunction(&m_pad, NotNullMatcher()));
        EXPECT_CALL(*m_gstWrapperMock, gstPadSetActive(&m_pad, TRUE));
        EXPECT_CALL(*m_gstWrapperMock, gstElementAddPad(GST_ELEMENT(&m_rialtoSrc), &m_pad));

        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_target));
        EXPECT_CALL(*m_glibWrapperMock, gFree(CharStrMatcher(m_name)));
    }

    void expectSyncElement(GstElement *expectedElement)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSyncStateWithParent(expectedElement));
    }

    void expectLinkDecryptor(GstElement *expectedSrcElement)
    {
        EXPECT_CALL(*m_decryptorFactoryMock,
                    createDecryptorElement(_, reinterpret_cast<IDecryptionService *>(m_decryptionServiceMock.get()), _))
            .WillOnce(Return(&m_decryptor));
        expectBin(&m_decryptor);
        expectSyncElement(&m_decryptor);
        EXPECT_CALL(*m_gstWrapperMock, gstElementLink(expectedSrcElement, &m_decryptor));
    }

    void expectLinkPayloader(GstElement *expectedSrcElement)
    {
        EXPECT_CALL(*m_glibWrapperMock, gOnceInitEnter(_)).WillOnce(Return(TRUE));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("svppay")))
            .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_factory)));
        EXPECT_CALL(*m_glibWrapperMock, gOnceInitLeave(_, 1));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_factory), _))
            .WillOnce(Return(&m_payloader));

        expectBin(&m_payloader);
        expectSyncElement(&m_payloader);
        EXPECT_CALL(*m_gstWrapperMock, gstElementLink(expectedSrcElement, &m_payloader));
    }

    void expectLinkQueue(GstElement *expectedSrcElement)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("queue"), _)).WillOnce(Return(&m_queue));

        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_queue, CharStrMatcher("max-size-buffers")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_queue, CharStrMatcher("max-size-bytes")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_queue, CharStrMatcher("max-size-time")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_queue, CharStrMatcher("silent")));

        expectBin(&m_queue);
        expectSyncElement(&m_queue);
        EXPECT_CALL(*m_gstWrapperMock, gstElementLink(expectedSrcElement, &m_queue));
    }
};

/**
 * Test that GstSrc can add and setup a video source.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, SetupVideo)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.m_appSrc);
    expectSyncElement(m_streamInfo.m_appSrc);
    expectLinkDecryptor(m_streamInfo.m_appSrc);
    expectLinkPayloader(&m_decryptor);
    expectLinkQueue(&m_payloader);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppArc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc can add and setup a audio source.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, SetupAudio)
{
    guint64 audioMaxBytes = 512 * 1024;

    expectSettings(audioMaxBytes);
    expectBin(m_streamInfo.m_appSrc);
    expectSyncElement(m_streamInfo.m_appSrc);
    expectLinkDecryptor(m_streamInfo.m_appSrc);
    expectLinkQueue(&m_decryptor);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppArc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::AUDIO);
}

/**
 * Test that GstSrc still created if decryptor element fails.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, DecryptorFailure)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    EXPECT_CALL(*m_decryptorFactoryMock,
                createDecryptorElement(_, reinterpret_cast<IDecryptionService *>(m_decryptionServiceMock.get()), _))
        .WillOnce(Return(nullptr));

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.m_appSrc);
    expectSyncElement(m_streamInfo.m_appSrc);
    expectLinkPayloader(m_streamInfo.m_appSrc);
    expectLinkQueue(&m_payloader);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppArc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc still created if payloader element fails.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, PayloaderFailure)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    EXPECT_CALL(*m_glibWrapperMock, gOnceInitEnter(_)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("svppay")))
        .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_factory)));
    EXPECT_CALL(*m_glibWrapperMock, gOnceInitLeave(_, 1));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_factory), _))
        .WillOnce(Return(nullptr));

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.m_appSrc);
    expectSyncElement(m_streamInfo.m_appSrc);
    expectLinkDecryptor(m_streamInfo.m_appSrc);
    expectLinkQueue(&m_decryptor);
    expectSetupPad(&m_queue);

    m_gstSrc->setupAndAddAppArc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
                                this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc still created if queue element fails.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, QueueFailure)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("queue"), _)).WillOnce(Return(nullptr));

    expectSettings(videoMaxBytes);
    expectBin(m_streamInfo.m_appSrc);
    expectSyncElement(m_streamInfo.m_appSrc);
    expectLinkDecryptor(m_streamInfo.m_appSrc);
    expectLinkPayloader(&m_decryptor);
    expectSetupPad(&m_payloader);

    m_gstSrc->setupAndAddAppArc(m_decryptionServiceMock.get(), GST_ELEMENT(&m_rialtoSrc), m_streamInfo, &m_callbacks,
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
