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

#include <memory>
#include <string>

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "MessageBuilders.h"
#include "WebAudioTestCommon.h"
#include "WebAudioTestMethods.h"

using testing::_;
using testing::AtLeast;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::StrEq;

using ::firebolt::rialto::WebAudioPlayerStateEvent;

namespace
{
constexpr int kPcmRate{41000};
constexpr int kPcmChannels{2};
constexpr int kPcmSampleSize{16};
constexpr bool kPcmIsBigEndian{true};
constexpr bool kPcmIsSigned{true};
constexpr bool kPcmIsFloat{false};
const std::string kAudioMimeType{"audio/x-raw"};
constexpr int kTestDelayFrames{191};
constexpr double kTestVolume1{0.31};
constexpr double kTestVolume2{0.51};
} // namespace

namespace firebolt::rialto::server::ct
{
WebAudioTestMethods::WebAudioTestMethods()
{
    memset(&m_appSrc, 0x00, sizeof(m_appSrc));
    memset(&m_pipeline, 0x00, sizeof(m_pipeline));
    memset(&m_reg, 0x00, sizeof(m_reg));
    memset(&m_feature, 0x00, sizeof(m_feature));
    memset(&m_sink, 0x00, sizeof(m_sink));
    memset(&m_bus, 0x00, sizeof(m_bus));
    memset(&m_gstCaps1, 0x00, sizeof(m_gstCaps1));
    memset(&m_gstCaps2, 0x00, sizeof(m_gstCaps2));
    memset(&m_convert, 0x00, sizeof(m_convert));
    memset(&m_resample, 0x00, sizeof(m_resample));
    memset(&m_volume, 0x00, sizeof(m_volume));
    memset(&m_buffer, 0x00, sizeof(m_buffer));
    memset(&m_capsStr, 0x00, sizeof(m_capsStr));

    configureSutInActiveState();
    connectClient();
}

WebAudioTestMethods::~WebAudioTestMethods() {}

void WebAudioTestMethods::initShm()
{
    auto getShmReq{createGetSharedMemoryRequest()};
    ConfigureAction<GetSharedMemory>(m_clientStub)
        .send(getShmReq)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { m_shmHandle.init(resp.fd(), resp.size()); });
}

int WebAudioTestMethods::checkInitialBufferAvailable()
{
    int offsetMain;
    int lengthMain;
    int offsetWrap;
    int lengthWrap;
    webAudioGetBufferAvailable(offsetMain, lengthMain, offsetWrap, lengthWrap);

    constexpr int kExpectedMinBytes{1024 * 1024}; // one MB
    constexpr int kExpectedMinLengthMain{2560};
    EXPECT_LE(kExpectedMinBytes, offsetMain);
    EXPECT_LE(kExpectedMinLengthMain, lengthMain);
    EXPECT_LE(kExpectedMinBytes, offsetWrap);
    EXPECT_EQ(0, lengthWrap);

    return lengthMain + lengthWrap;
}

int WebAudioTestMethods::getBufferAvailable()
{
    int offsetMain;
    int lengthMain;
    int offsetWrap;
    int lengthWrap;
    webAudioGetBufferAvailable(offsetMain, lengthMain, offsetWrap, lengthWrap);

    return lengthMain + lengthWrap;
}

void WebAudioTestMethods::sendDataToShm(int len)
{
    const char *kData = "`1909f3qVREGPcwe[m4jH";
    int offsetMain;
    int lengthMain;
    int offsetWrap;
    int lengthWrap;
    webAudioGetBufferAvailable(offsetMain, lengthMain, offsetWrap, lengthWrap);

    const char *kPtrFrom = kData;
    auto ptrTo = m_shmHandle.getShm() + offsetMain;
    if (len < lengthMain)
    {
        for (int i = 0; i < len; ++i)
        {
            *ptrTo = *kPtrFrom;
            m_dataFifo.push(*kPtrFrom);
            ++ptrTo;
            if (*kPtrFrom == 0)
                kPtrFrom = kData;
            else
                ++kPtrFrom;
        }
    }
    else
    {
        for (int i = 0; i < lengthMain; ++i)
        {
            *ptrTo = *kPtrFrom;
            m_dataFifo.push(*kPtrFrom);
            ++ptrTo;
            if (*kPtrFrom == 0)
                kPtrFrom = kData;
            else
                ++kPtrFrom;
        }
        len -= lengthMain;
        ptrTo = m_shmHandle.getShm() + offsetWrap;
        for (int i = 0; i < len; ++i)
        {
            *ptrTo = *kPtrFrom;
            m_dataFifo.push(*kPtrFrom);
            ++ptrTo;
            if (*kPtrFrom == 0)
                kPtrFrom = kData;
            else
                ++kPtrFrom;
        }
    }
}

void WebAudioTestMethods::willCreateWebAudioPlayer()
{
    constexpr uint64_t kChannelMask{5};

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("rialtosrc"))).WillOnce(Return(nullptr));

    EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(nullptr, StrEq("rialtosrc"), _, _)).WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstPipelineNew(StrEq("webaudiopipeline3"))).WillOnce(Return(&m_pipeline));

    // EXPECTS coming from...
    //   GstWebAudioPlayerTestCommon::expectInitAppSrc()
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(&m_appSrc, 10 * 1024));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_appSrc), StrEq("format")));

    // EXPECTS coming from...
    //   GstWebAudioPlayerTestCommon::expectMakeAmlhalaSink()
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet()).WillOnce(Return(&m_reg));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, StrEq("amlhalasink")))
        .WillOnce(Return(GST_PLUGIN_FEATURE(&m_feature)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("amlhalasink"), StrEq("webaudiosink")))
        .WillOnce(Return(&m_sink));

    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, StrEq("direct-mode")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_PLUGIN_FEATURE(&m_feature)));

    // EXPECTS coming from...
    //   GstWebAudioPlayerTestCommon::expectInitAppSrc()
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("appsrc"), StrEq("audsrc")))
        .WillOnce(Return(GST_ELEMENT(&m_appSrc)));

    // EXPECTS coming from...
    //   GstWebAudioPlayerTestCommon::expectLinkElements()
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("audioconvert"), _)).WillOnce(Return(&m_convert));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("audioresample"), _)).WillOnce(Return(&m_resample));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("volume"), _)).WillOnce(Return(&m_volume));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), GST_ELEMENT(&m_appSrc))).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_convert)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_resample)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_volume)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_sink)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(GST_ELEMENT(&m_appSrc), &m_convert)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&m_convert, &m_resample)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&m_resample, &m_volume)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&m_volume, &m_sink)).WillOnce(Return(TRUE));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq(kAudioMimeType))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("channels"), G_TYPE_INT, kPcmChannels));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("rate"), G_TYPE_INT, kPcmRate));

    ///////////////////////////////////////////////
    // The following EXPECTS are generated from GstDispatcherThread::gstBusEventHandler()
    // GstWebAudioPlayerTestCommon::expectTermPipeline ??
    m_gstreamerStub.setupMessages(true);
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(_))
        .WillRepeatedly(Invoke(
            [&](GstState state) -> const gchar *
            {
                switch (state)
                {
                case GST_STATE_READY:
                    return "Ready";
                case GST_STATE_PLAYING:
                    return "Playing";
                case GST_STATE_PAUSED:
                    return "Paused";
                case GST_STATE_NULL:
                    return "Null";
                case GST_STATE_VOID_PENDING:
                    return "Void pending";
                default:
                    return "Unknown";
                }
                return nullptr;
            }));

    // Similar to EXPECTS in...
    //    WebAudioTasksTestsBase::shouldBuildPcmCaps
    const std::string kFormatStr{testcommon::getPcmFormat(kPcmIsFloat, kPcmIsSigned, kPcmSampleSize, kPcmIsBigEndian)};
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("format"), G_TYPE_STRING, StrEq(kFormatStr)));

    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleStringStub(&m_gstCaps1, StrEq("layout"), G_TYPE_STRING, StrEq("interleaved")));

    EXPECT_CALL(*m_gstWrapperMock, gstAudioChannelGetFallbackMask(kPcmChannels)).WillOnce(Return(kChannelMask));

    // Similar to EXPECTS in...
    //   WebAudioTasksTestsBase::shouldBuildPcmCaps()
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleBitMaskStub(&m_gstCaps1, StrEq("channel-mask"), GST_TYPE_BITMASK, kChannelMask));

    // Similar to EXPECTS in...
    //   WebAudioTasksTestsBase::shouldGetCapsStr()
    EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&m_gstCaps1)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapperMock, gFree(&m_capsStr));

    // Similar to EXPECTS in...
    //   WebAudioTasksTestsBase::shouldNotSetCapsWhenCapsEqual()
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsEqual(&m_gstCaps2, &m_gstCaps1)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(&m_appSrc)).WillOnce(Return(&m_gstCaps2));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_gstCaps2));

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus)).Times(2);

    // EXPECTS coming from...
    //   GstWebAudioPlayer::changePipelineState
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_NULL))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));

    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pipeline));
}

void WebAudioTestMethods::createWebAudioPlayer()
{
    constexpr int kPriority{3};
    auto request = createCreateWebAudioPlayerRequest(kPcmRate, kPcmChannels, kPcmSampleSize, kPcmIsBigEndian,
                                                     kPcmIsSigned, kPcmIsFloat, kAudioMimeType, kPriority);

    ConfigureAction<CreateWebAudioPlayer>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { m_webAudioPlayerHandle = resp.web_audio_player_handle(); });

    EXPECT_GE(m_webAudioPlayerHandle, 0);
}

void WebAudioTestMethods::willFailToCreateWebAudioPlayer()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("rialtosrc"))).WillOnce(Return(nullptr));

    EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(nullptr, StrEq("rialtosrc"), _, _)).WillOnce(Return(false));

    EXPECT_CALL(*m_gstWrapperMock, gstPipelineNew(StrEq("webaudiopipeline3"))).WillOnce(Return(nullptr));
}

void WebAudioTestMethods::failToCreateWebAudioPlayer()
{
    constexpr int kPriority{3};
    auto request = createCreateWebAudioPlayerRequest(kPcmRate, kPcmChannels, kPcmSampleSize, kPcmIsBigEndian,
                                                     kPcmIsSigned, kPcmIsFloat, kAudioMimeType, kPriority);

    ConfigureAction<CreateWebAudioPlayer>(m_clientStub).send(request).expectFailure();
}

void WebAudioTestMethods::destroyWebAudioPlayer()
{
    if (m_webAudioPlayerHandle >= 0)
    {
        auto request = createDestroyWebAudioPlayerRequest(m_webAudioPlayerHandle);

        ConfigureAction<DestroyWebAudioPlayer>(m_clientStub).send(request).expectSuccess();
        m_webAudioPlayerHandle = -1;
    }
}

void WebAudioTestMethods::willWebAudioPlay()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PLAYING))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
}

void WebAudioTestMethods::willWebAudioPlayFail()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PLAYING))
        .WillOnce(Return(GST_STATE_CHANGE_FAILURE));
}

void WebAudioTestMethods::webAudioPlay()
{
    EXPECT_GE(m_webAudioPlayerHandle, 0);
    auto request = createWebAudioPlayRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioPlay>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTestMethods::willWebAudioPause()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PAUSED))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
}

void WebAudioTestMethods::willWebAudioPauseFail()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PAUSED))
        .WillOnce(Return(GST_STATE_CHANGE_FAILURE));
}

void WebAudioTestMethods::webAudioPause()
{
    EXPECT_GE(m_webAudioPlayerHandle, 0);
    auto request = createWebAudioPauseRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioPause>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTestMethods::willWebAudioSetEos()
{
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcEndOfStream(&m_appSrc)).WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTestMethods::webAudioSetEos()
{
    EXPECT_GE(m_webAudioPlayerHandle, 0);
    auto request = createWebAudioSetEosRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioSetEos>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTestMethods::webAudioGetBufferAvailable(int &offsetMain, int &lengthMain, int &offsetWrap, int &lengthWrap)
{
    EXPECT_GE(m_webAudioPlayerHandle, 0);
    auto request = createWebAudioGetBufferAvailableRequest(m_webAudioPlayerHandle);

    ConfigureAction<WebAudioGetBufferAvailable>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const auto &resp)
            {
                auto x = resp.shm_info();
                offsetMain = x.offset_main();
                lengthMain = x.length_main();
                offsetWrap = x.offset_wrap();
                lengthWrap = x.length_wrap();
            });
}

void WebAudioTestMethods::willWebAudioGetBufferDelay()
{
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCurrentLevelBytes(&m_appSrc)).WillRepeatedly(Return(kTestDelayFrames * 4));
}

void WebAudioTestMethods::webAudioGetBufferDelay()
{
    EXPECT_GE(m_webAudioPlayerHandle, 0);
    auto request = createWebAudioGetBufferDelayRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioGetBufferDelay>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.delay_frames(), kTestDelayFrames); });
}

void WebAudioTestMethods::willWebAudioWriteBuffer(int len)
{
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCurrentLevelBytes(&m_appSrc)).WillRepeatedly(Return(kTestDelayFrames * 4));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, _, nullptr)).WillRepeatedly(Return(&m_buffer));

    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&m_buffer, _, _, _))
        .WillRepeatedly(Invoke(
            [&](GstBuffer *buffer, gsize offset, gconstpointer src, gsize size) -> gsize
            {
                const std::uint8_t *kSrcPtr = static_cast<const std::uint8_t *>(src);
                for (gsize i = 0; i < size; ++i)
                {
                    EXPECT_FALSE(m_dataFifo.empty());
                    EXPECT_EQ(kSrcPtr[i], m_dataFifo.front());
                    m_dataFifo.pop();
                }
                return size;
            }));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_appSrc, &m_buffer)).WillRepeatedly(Return(GST_FLOW_OK));
}

void WebAudioTestMethods::webAudioWriteBuffer(int len)
{
    constexpr int kFrameLen{4}; // 2 bytes per sample and 2 channels
    int lenFrames = len / kFrameLen;
    EXPECT_GE(m_webAudioPlayerHandle, 0);
    auto request = createWebAudioWriteBufferRequest(m_webAudioPlayerHandle, lenFrames);
    ConfigureAction<WebAudioWriteBuffer>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTestMethods::webAudioGetDeviceInfo()
{
    EXPECT_GE(m_webAudioPlayerHandle, 0);
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

void WebAudioTestMethods::willWebAudioSetVolume()
{
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeSetVolume(_, _, kTestVolume1));
}

void WebAudioTestMethods::webAudioSetVolume()
{
    EXPECT_GE(m_webAudioPlayerHandle, 0);
    auto request = createWebAudioSetVolumeRequest(m_webAudioPlayerHandle, kTestVolume1);
    ConfigureAction<WebAudioSetVolume>(m_clientStub).send(request).expectSuccess();
}

void WebAudioTestMethods::willWebAudioGetVolume()
{
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeGetVolume(_, _)).WillOnce(Return(kTestVolume2));
}

void WebAudioTestMethods::webAudioGetVolume()
{
    EXPECT_GE(m_webAudioPlayerHandle, 0);
    auto request = createWebAudioGetVolumeRequest(m_webAudioPlayerHandle);
    ConfigureAction<WebAudioGetVolume>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.volume(), kTestVolume2); });
}

void WebAudioTestMethods::sendStateChanged(GstState oldState, GstState newState, GstState pendingState)
{
    EXPECT_CALL(*m_gstWrapperMock, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
    if (!m_expectedMessage)
    {
        m_expectedMessage = std::make_unique<ExpectMessage<WebAudioPlayerStateEvent>>(m_clientStub);
    }
    m_gstreamerStub.sendStateChanged(oldState, newState, pendingState, true);
}

void WebAudioTestMethods::checkMessageReceivedForStateChange(
    firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState expect)
{
    EXPECT_GE(m_webAudioPlayerHandle, 0);
    auto message = m_expectedMessage->getMessage();
    ASSERT_TRUE(message);
    ASSERT_EQ(message->web_audio_player_handle(), m_webAudioPlayerHandle);
    ASSERT_EQ(message->state(), expect);

    // Check that there isn't a subsequent message (this is expected to timeout)
    // and therefore message will be null
    m_expectedMessage = std::make_unique<ExpectMessage<WebAudioPlayerStateEvent>>(m_clientStub);
    message = m_expectedMessage->getMessage();
    ASSERT_FALSE(message);
}

void WebAudioTestMethods::willGstSendEos()
{
    EXPECT_CALL(*m_gstWrapperMock, gstEventNewFlushStart()).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstEventNewFlushStop(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSendEvent(_, _)).WillRepeatedly(Return(true));
}

void WebAudioTestMethods::gstSendEos()
{
    m_gstreamerStub.sendEos();
}
} // namespace firebolt::rialto::server::ct
