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

#include "GlibWrapperFactoryMock.h"
#include "GlibWrapperMock.h"
#include "GstSrc.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
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
    std::unique_ptr<IGstSrc> m_gstSrc;

    GstElement m_appsrc = {};
    GstBin m_rialtoSrcBin = {};
    GstRialtoSrcPrivate m_rialtoSrcPriv = {};
    GstRialtoSrc m_rialtoSrc = {m_rialtoSrcBin, &m_rialtoSrcPriv};
    GstAppSrcCallbacks m_callbacks = {};
    GstPad m_target = {};
    GstPad m_pad = {};
    gchar *m_name = "src_0";

    virtual void SetUp()
    {
        m_gstWrapperFactoryMock = std::make_shared<StrictMock<GstWrapperFactoryMock>>();
        m_gstWrapperMock = std::make_shared<StrictMock<GstWrapperMock>>();

        m_glibWrapperFactoryMock = std::make_shared<StrictMock<GlibWrapperFactoryMock>>();
        m_glibWrapperMock = std::make_shared<StrictMock<GlibWrapperMock>>();

        createGstSrc();
    }

    virtual void TearDown()
    {
        m_gstSrc.reset();

        m_glibWrapperMock.reset();
        m_glibWrapperFactoryMock.reset();

        m_gstWrapperMock.reset();
        m_gstWrapperFactoryMock.reset();
    }

    void createGstSrc()
    {
        EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));
        EXPECT_CALL(*m_glibWrapperFactoryMock, getGlibWrapper()).WillOnce(Return(m_glibWrapperMock));

        EXPECT_NO_THROW(m_gstSrc = std::make_unique<GstSrc>(m_gstWrapperFactoryMock, m_glibWrapperFactoryMock););
        EXPECT_NE(m_gstSrc, nullptr);
    }

    void expectSettings(guint64 max)
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_appsrc, CharStrMatcher("block")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_appsrc, CharStrMatcher("format")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_appsrc, CharStrMatcher("stream-type")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_appsrc, CharStrMatcher("min-percent")));

        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCallbacks(GST_APP_SRC(&m_appsrc), &m_callbacks, this, nullptr));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(GST_APP_SRC(&m_appsrc), max));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetStreamType(GST_APP_SRC(&m_appsrc), GST_APP_STREAM_TYPE_SEEKABLE));
    }

    void expectBin() { EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_rialtoSrc), &m_appsrc)); }

    void expectSetupPad()
    {
        EXPECT_CALL(*m_glibWrapperMock, gStrdupPrintfStub(_)).WillOnce(Return(m_name));

        EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(&m_appsrc, CharStrMatcher("src"))).WillOnce(Return(&m_target));
        EXPECT_CALL(*m_gstWrapperMock, gstGhostPadNew(CharStrMatcher(m_name), &m_target)).WillOnce(Return(&m_pad));
        EXPECT_CALL(*m_gstWrapperMock, gstPadSetQueryFunction(&m_pad, NotNullMatcher()));
        EXPECT_CALL(*m_gstWrapperMock, gstPadSetActive(&m_pad, TRUE));
        EXPECT_CALL(*m_gstWrapperMock, gstElementAddPad(GST_ELEMENT(&m_rialtoSrc), &m_pad));

        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_target));
        EXPECT_CALL(*m_glibWrapperMock, gFree(CharStrMatcher(m_name)));
    }

    void expectSyncElement() { EXPECT_CALL(*m_gstWrapperMock, gstElementSyncStateWithParent(&m_appsrc)); }
};

/**
 * Test that GstSrc can add and setup a video source.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, SetupVideo)
{
    guint64 videoMaxBytes = 8 * 1024 * 1024;

    expectSettings(videoMaxBytes);
    expectBin();
    expectSetupPad();
    expectSyncElement();

    m_gstSrc->setupAndAddAppArc(GST_ELEMENT(&m_rialtoSrc), &m_appsrc, &m_callbacks, this, MediaSourceType::VIDEO);
}

/**
 * Test that GstSrc can add and setup a audio source.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, SetupAudio)
{
    guint64 audioMaxBytes = 512 * 1024;

    expectSettings(audioMaxBytes);
    expectBin();
    expectSetupPad();
    expectSyncElement();

    m_gstSrc->setupAndAddAppArc(GST_ELEMENT(&m_rialtoSrc), &m_appsrc, &m_callbacks, this, MediaSourceType::AUDIO);
}

/**
 * Test that GstSrc can signal all app sources are added.
 */
TEST_F(RialtoServerAppSrcGstSrcTest, AllSrcsAdded)
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementNoMorePads(GST_ELEMENT(&m_rialtoSrc)));

    m_gstSrc->allAppSrcsAdded(GST_ELEMENT(&m_rialtoSrc));
}
