/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "WebAudioTasksTestsBase.h"

class WebAudioSetCapsTest : public WebAudioTasksTestsBase
{
protected:
    const std::string m_kAudioMimeType{"audio/x-raw"};
    firebolt::rialto::WebAudioConfig m_config;
    GstCaps m_caps{};
    GstCaps m_capsAppSrc{};
    GstElement m_appSrc{};
    gchar m_capsStr{};
    const uint64_t m_channelMask{5};

    WebAudioSetCapsTest()
    {
        m_config.pcm.rate = 1;
        m_config.pcm.channels = 2;
        m_config.pcm.sampleSize = 32;
        m_config.pcm.isBigEndian = false;
        m_config.pcm.isSigned = false;
        m_config.pcm.isFloat = false;

        m_context.source = &m_appSrc;
    }

    std::string getPcmFormat()
    {
        std::string format;

        if (m_config.pcm.isFloat)
        {
            format += "F";
        }
        else if (m_config.pcm.isSigned)
        {
            format += "S";
        }
        else
        {
            format += "U";
        }

        format += std::to_string(m_config.pcm.sampleSize);

        if (m_config.pcm.isBigEndian)
        {
            format += "BE";
        }
        else
        {
            format += "LE";
        }

        return format;
    }

    void expectBuildPcmCaps()
    {
        EXPECT_CALL(*m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/x-raw"))).WillOnce(Return(&m_caps));
        EXPECT_CALL(*m_gstWrapper,
                    gstCapsSetSimpleIntStub(&m_caps, StrEq("channels"), G_TYPE_INT, m_config.pcm.channels));
        EXPECT_CALL(*m_gstWrapper,
                    gstCapsSetSimpleStringStub(&m_caps, StrEq("layout"), G_TYPE_STRING, StrEq("interleaved")));
        EXPECT_CALL(*m_gstWrapper, gstCapsSetSimpleIntStub(&m_caps, StrEq("rate"), G_TYPE_INT, m_config.pcm.rate));
        EXPECT_CALL(*m_gstWrapper,
                    gstCapsSetSimpleStringStub(&m_caps, StrEq("format"), G_TYPE_STRING, StrEq(getPcmFormat().c_str())));
        EXPECT_CALL(*m_gstWrapper, gstAudioChannelGetFallbackMask(m_config.pcm.channels)).WillOnce(Return(m_channelMask));
        EXPECT_CALL(*m_gstWrapper,
                    gstCapsSetSimpleBitMaskStub(&m_caps, StrEq("channel-mask"), GST_TYPE_BITMASK, m_channelMask));
    }

    void expectGetCapsStr()
    {
        EXPECT_CALL(*m_gstWrapper, gstCapsToString(&m_caps)).WillOnce(Return(&m_capsStr));
        EXPECT_CALL(*m_glibWrapper, gFree(&m_capsStr));
    }

    void expectSetCaps()
    {
        EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(&m_capsAppSrc));
        EXPECT_CALL(*m_gstWrapper, gstCapsIsEqual(&m_capsAppSrc, &m_caps)).WillOnce(Return(FALSE));
        EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_caps));
    }

    void expectUnref()
    {
        EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_capsAppSrc));
        EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_caps));
    }

    void expectSetBytesPerSamplePcm()
    {
        uint32_t expectedBytesPerSample = m_config.pcm.channels * (m_config.pcm.sampleSize / CHAR_BIT);
        EXPECT_EQ(expectedBytesPerSample, m_context.bytesPerSample);
    }
};

TEST_F(WebAudioSetCapsTest, shouldSetCapsWithFormatF64LE)
{
    m_config.pcm.sampleSize = 64;
    m_config.pcm.isFloat = true;

    expectBuildPcmCaps();
    expectGetCapsStr();
    expectSetCaps();
    expectUnref();

    firebolt::rialto::server::tasks::webaudio::SetCaps task{m_context, m_gstWrapper, m_glibWrapper, m_kAudioMimeType,
                                                            &m_config};
    task.execute();
    expectSetBytesPerSamplePcm();
}

TEST_F(WebAudioSetCapsTest, shouldSetCapsWithWithFormatS16BE)
{
    m_config.pcm.sampleSize = 16;
    m_config.pcm.isSigned = true;
    m_config.pcm.isBigEndian = true;

    expectBuildPcmCaps();
    expectGetCapsStr();
    expectSetCaps();
    expectUnref();

    firebolt::rialto::server::tasks::webaudio::SetCaps task{m_context, m_gstWrapper, m_glibWrapper, m_kAudioMimeType,
                                                            &m_config};
    task.execute();
    expectSetBytesPerSamplePcm();
}

TEST_F(WebAudioSetCapsTest, shouldSetCapsWithFormatU32LE)
{
    expectBuildPcmCaps();
    expectGetCapsStr();
    expectSetCaps();
    expectUnref();

    firebolt::rialto::server::tasks::webaudio::SetCaps task{m_context, m_gstWrapper, m_glibWrapper, m_kAudioMimeType,
                                                            &m_config};
    task.execute();
    expectSetBytesPerSamplePcm();
}

TEST_F(WebAudioSetCapsTest, shouldSetCapsWhenAppSrcCapsNull)
{
    expectBuildPcmCaps();
    expectGetCapsStr();
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&m_appSrc), &m_caps));
    EXPECT_CALL(*m_gstWrapper, gstCapsUnref(&m_caps));

    firebolt::rialto::server::tasks::webaudio::SetCaps task{m_context, m_gstWrapper, m_glibWrapper, m_kAudioMimeType,
                                                            &m_config};
    task.execute();
}

TEST_F(WebAudioSetCapsTest, shouldNotSetCapsWhenInvalidMimeType)
{
    firebolt::rialto::server::tasks::webaudio::SetCaps task{m_context, m_gstWrapper, m_glibWrapper, "invalid", &m_config};
    task.execute();
}

TEST_F(WebAudioSetCapsTest, shouldNotSetCapsWhenCapsEqual)
{
    expectBuildPcmCaps();
    expectGetCapsStr();
    expectUnref();
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(&m_capsAppSrc));
    EXPECT_CALL(*m_gstWrapper, gstCapsIsEqual(&m_capsAppSrc, &m_caps)).WillOnce(Return(TRUE));

    firebolt::rialto::server::tasks::webaudio::SetCaps task{m_context, m_gstWrapper, m_glibWrapper, m_kAudioMimeType,
                                                            &m_config};
    task.execute();
}
