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

#include <vector>

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"
#include "SharedMemoryBuffer.h"
#include "WebAudioTestCommon.h"

using testing::_;
using testing::AtLeast;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

using ::google::protobuf::int32;
using ::google::protobuf::uint32;

namespace firebolt::rialto::server::ct
{
class WebAudioTest : public RialtoServerComponentTest
{
public:
    WebAudioTest()
    {
        willConfigureSocket();
        configureSutInActiveState();
        connectClient();
    }
    virtual ~WebAudioTest() {}

    void willCreateWebAudioPlayer();
    void createWebAudioPlayer();

    void willWebAudioPlay();
    void webAudioPlay();

    void willWebAudioPause();
    void webAudioPause();

    void willWebAudioSetEos();
    void webAudioSetEos();

    void webAudioGetBufferAvailable();

    void willWebAudioGetBufferDelay();
    void webAudioGetBufferDelay();

    void willWebAudioWriteBuffer();
    void webAudioWriteBuffer();

    void webAudioGetDeviceInfo();

    void willWebAudioSetVolume();
    void webAudioSetVolume();

    void willWebAudioGetVolume();
    void webAudioGetVolume();

    void destroyWebAudioPlayer();

protected:
    int32 m_webAudioPlayerHandle;

    GstElement m_pipeline{};
    GstElement m_appSrc{};
    GstRegistry m_reg{};
    GstObject m_feature{};
    GstElement m_sink{};
    GstBus m_bus{};
    GstMessage m_message{};
    GstCaps m_gstCaps1{};
    GstCaps m_gstCaps2{};
    GstElement m_convert{};
    GstElement m_resample{};
    gchar m_capsStr{};
    GstBuffer m_buffer;

    const uint32 m_kPcmRate{41000};
    const uint32 m_kPcmChannels{2};
    const uint32 m_kPcmSampleSize{16};
    const bool m_kPcmIsBigEndian{true};
    const bool m_kPcmIsSigned{true};
    const bool m_kPcmIsFloat{false};
    const std::string m_kAudioMimeType{"audio/x-raw"};
    const int m_kTestDelayFrames{191};
    const int m_kTestBufLen{43};
    const double m_kTestVolume1{0.31};
    const double m_kTestVolume2{0.51};
};

void WebAudioTest::willCreateWebAudioPlayer()
{
    constexpr int kGetBusNumberOfCalls{2};
    constexpr uint64_t kChannelMask{5};

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("rialtosrc"))).WillOnce(Return(nullptr));

    EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(nullptr, StrEq("rialtosrc"), _, _)).WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstPipelineNew(StrEq("webaudiopipeline3"))).WillOnce(Return(&m_pipeline));

    // Similar to tests in the class GstWebAudioPlayerTestCommon

    // GstWebAudioPlayerTestCommon::expectInitAppSrc()
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(GST_APP_SRC(&m_appSrc), 10 * 1024));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_appSrc), StrEq("format")));

    // GstWebAudioPlayerTestCommon::expectMakeAmlhalaSink()
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet()).WillOnce(Return(&m_reg));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, StrEq("amlhalasink")))
        .WillOnce(Return(GST_PLUGIN_FEATURE(&m_feature)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("amlhalasink"), StrEq("webaudiosink")))
        .WillOnce(Return(&m_sink));

    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, StrEq("direct-mode")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_PLUGIN_FEATURE(&m_feature)));

    // From GstWebAudioPlayerTestCommon::expectInitAppSrc()
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("appsrc"), StrEq("audsrc"))).WillOnce(Return(&m_appSrc));

    // From GstWebAudioPlayerTestCommon::expectLinkElements()
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("audioconvert"), _)).WillOnce(Return(&m_convert));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("audioresample"), _)).WillOnce(Return(&m_resample));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_appSrc)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_convert)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_resample)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_sink)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&m_appSrc, &m_convert)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&m_convert, &m_resample)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&m_resample, &m_sink)).WillOnce(Return(TRUE));

    // Similar to GenericTasksTestsBase::shouldAttachAudioSourceWithChannelsAndRate()
    EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq(m_kAudioMimeType))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("channels"), G_TYPE_INT, m_kPcmChannels));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("rate"), G_TYPE_INT, m_kPcmRate));

    ///////////////////////////////////////////////
    // The following EXPECTS are generated from GstDispatcherThread::gstBusEventHandler()
    // GstWebAudioPlayerTestCommon::expectTermPipeline ??
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline)))
        .Times(kGetBusNumberOfCalls)
        .WillRepeatedly(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusTimedPopFiltered(&m_bus, 100 * GST_MSECOND, _)).WillRepeatedly(Return(&m_message));

    // WebAudioTasksTestsBase::shouldBuildPcmCaps ??
    std::string formatStr{testcommon::getPcmFormat(m_kPcmIsFloat, m_kPcmIsSigned, m_kPcmSampleSize, m_kPcmIsBigEndian)};
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("format"), G_TYPE_STRING, StrEq(formatStr)));

    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("layout"), G_TYPE_STRING, StrEq("interleaved")));

    EXPECT_CALL(*m_gstWrapperMock, gstAudioChannelGetFallbackMask(m_kPcmChannels)).WillOnce(Return(kChannelMask));

    // WebAudioTasksTestsBase::shouldBuildPcmCaps()
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleBitMaskStub(&m_gstCaps1, StrEq("channel-mask"), GST_TYPE_BITMASK, kChannelMask));

    // WebAudioTasksTestsBase::shouldGetCapsStr()
    EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapperMock, gFree(&m_capsStr));

    // WebAudioTasksTestsBase::shouldNotSetCapsWhenCapsEqual()
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsEqual(&m_gstCaps2, &m_gstCaps1)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&m_appSrc))).WillOnce(Return(&m_gstCaps2));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_gstCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstMessageUnref(&m_message)).Times(AtLeast(1));

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus)).Times(kGetBusNumberOfCalls);
    // The above EXPECTS were generated from GstDispatcherThread::gstBusEventHandler()
    ///////////////////////////////////////////////

    // Generated from GstWebAudioPlayer::changePipelineState
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_NULL))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));

    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pipeline));
}

void WebAudioTest::createWebAudioPlayer()
{
    constexpr uint32 kPriority{3};
    auto request = createCreateWebAudioPlayerRequest(m_kPcmRate, m_kPcmChannels, m_kPcmSampleSize, m_kPcmIsBigEndian,
                                                     m_kPcmIsSigned, m_kPcmIsFloat, m_kAudioMimeType, kPriority);

    ConfigureAction<CreateWebAudioPlayer>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { m_webAudioPlayerHandle = resp.web_audio_player_handle(); });
}

void WebAudioTest::destroyWebAudioPlayer()
{
    auto request = createDestroyWebAudioPlayerRequest(m_webAudioPlayerHandle);

    ConfigureAction<DestroyWebAudioPlayer>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTest::willWebAudioPlay()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PLAYING))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
}

void WebAudioTest::webAudioPlay()
{
    auto request = createWebAudioPlayRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioPlay>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTest::willWebAudioPause()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PAUSED))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
}

void WebAudioTest::webAudioPause()
{
    auto request = createWebAudioPauseRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioPause>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTest::willWebAudioSetEos()
{
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcEndOfStream(GST_APP_SRC(&m_appSrc))).WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTest::webAudioSetEos()
{
    auto request = createWebAudioSetEosRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioSetEos>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTest::webAudioGetBufferAvailable()
{
    auto request = createWebAudioGetBufferAvailableRequest(m_webAudioPlayerHandle);
    constexpr uint32_t kExpectedMinBytes{1024 * 1024}; // one MB
    constexpr uint32_t kExpectedMinFrames{640};
    constexpr uint32_t kExpectedMinLengthMain{2560};

    ConfigureAction<WebAudioGetBufferAvailable>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const auto &resp)
            {
                auto x = resp.shm_info();
                EXPECT_LE(kExpectedMinBytes, x.offset_main());
                EXPECT_LE(kExpectedMinBytes, x.offset_wrap());
                EXPECT_LE(kExpectedMinFrames, resp.available_frames());
                EXPECT_LE(kExpectedMinLengthMain, x.length_main());
                EXPECT_EQ(0, x.length_wrap());
            });
}

void WebAudioTest::willWebAudioGetBufferDelay()
{
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&m_appSrc)))
        .WillOnce(Return(m_kTestDelayFrames * 4));
}

void WebAudioTest::webAudioGetBufferDelay()
{
    auto request = createWebAudioGetBufferDelayRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioGetBufferDelay>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.delay_frames(), m_kTestDelayFrames); });
}

void WebAudioTest::willWebAudioWriteBuffer()
{
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&m_appSrc)))
        .WillOnce(Return(m_kTestDelayFrames * 4));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, m_kTestBufLen * 4, nullptr)).WillOnce(Return(&m_buffer));

    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&m_buffer, 0, _, m_kTestBufLen * 4)).WillOnce(Return(m_kTestBufLen * 4));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(GST_APP_SRC(&m_appSrc), &m_buffer)).WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTest::webAudioWriteBuffer()
{
    auto request = createWebAudioWriteBufferRequest(m_webAudioPlayerHandle, m_kTestBufLen);
    ConfigureAction<WebAudioWriteBuffer>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTest::webAudioGetDeviceInfo()
{
    auto request = createWebAudioGetDeviceInfoRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioGetDeviceInfo>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const auto &resp)
            {
                // These values are (currently) returned from
                // WebAudioPlayerServerInternal::getDeviceInfo
                constexpr int kMaximumFramesMinValue{2560};
                constexpr int kPreferredFramesMinValue{640};

                EXPECT_GE(resp.maximum_frames(), kMaximumFramesMinValue);
                EXPECT_GE(resp.preferred_frames(), kPreferredFramesMinValue);
                EXPECT_TRUE(resp.support_deferred_play());
            });
}

void WebAudioTest::willWebAudioSetVolume()
{
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeSetVolume(_, _, m_kTestVolume1));
}

void WebAudioTest::webAudioSetVolume()
{
    auto request = createWebAudioSetVolumeRequest(m_webAudioPlayerHandle, m_kTestVolume1);
    ConfigureAction<WebAudioSetVolume>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTest::willWebAudioGetVolume()
{
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeGetVolume(_, _)).WillOnce(Return(m_kTestVolume2));
}

void WebAudioTest::webAudioGetVolume()
{
    auto request = createWebAudioGetVolumeRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioGetVolume>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.volume(), m_kTestVolume2); });
}

TEST_F(WebAudioTest, testAllApisWithMultipleQueries)
{
    willCreateWebAudioPlayer();
    createWebAudioPlayer();

    willWebAudioPlay();
    webAudioPlay();

    willWebAudioPause();
    webAudioPause();

    webAudioGetBufferAvailable();

    willWebAudioGetBufferDelay();
    webAudioGetBufferDelay();

    willWebAudioWriteBuffer();
    webAudioWriteBuffer();

    webAudioGetDeviceInfo();

    willWebAudioSetVolume();
    webAudioSetVolume();

    willWebAudioGetVolume();
    webAudioGetVolume();

    willWebAudioSetEos();
    webAudioSetEos();

    destroyWebAudioPlayer();
}
} // namespace firebolt::rialto::server::ct
