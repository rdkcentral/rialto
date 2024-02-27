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

#include <iostream>
using namespace std;

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ExpectMessage.h"
#include "GstSrc.h"
#include "GstreamerStub.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"
#include "SharedMemoryBuffer.h"
#include "WebAudioTestCommon.h"

using testing::_;
using testing::AtLeast;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::StrEq;

using ::google::protobuf::int32;
using ::google::protobuf::uint32;

using ::firebolt::rialto::WebAudioPlayerStateEvent;

namespace firebolt::rialto::server::ct
{
class WebAudioTest : public RialtoServerComponentTest
{
public:
    WebAudioTest()
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
        memset(&m_buffer, 0x00, sizeof(m_buffer));
        memset(&m_capsStr, 0x00, sizeof(m_capsStr));

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
    void sendStateChanged(GstState oldState, GstState newState, GstState pendingState);
    void checkMessageReceivedForStateChange(firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState expect);

    void willGstSendEos();
    void gstSendEos();

    unique_ptr<ExpectMessage<WebAudioPlayerStateEvent>> expectedMessage;

    int32 m_webAudioPlayerHandle;

    GstAppSrc m_appSrc;
    GstElement m_pipeline;
    GstRegistry m_reg;
    GstObject m_feature;
    GstElement m_sink;
    GstBus m_bus;
    GstCaps m_gstCaps1;
    GstCaps m_gstCaps2;
    GstElement m_convert;
    GstElement m_resample;
    GstBuffer m_buffer;
    gchar m_capsStr;

    GstBin m_rialtoSrcBin = {};
    GstRialtoSrcPrivate m_rialtoSrcPriv = {};
    GstRialtoSrc m_rialtoSource = {m_rialtoSrcBin, &m_rialtoSrcPriv};
    GstreamerStub m_gstreamerStub{m_glibWrapperMock, m_gstWrapperMock, &m_pipeline, &m_bus, GST_ELEMENT(&m_rialtoSource)};

    // Mock messages from gstreamer
    // GstMessage m_message;

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
    constexpr uint64_t kChannelMask{5};

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("rialtosrc"))).WillOnce(Return(nullptr));

    EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(nullptr, StrEq("rialtosrc"), _, _)).WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstPipelineNew(StrEq("webaudiopipeline3"))).WillOnce(Return(&m_pipeline));

    // Similar to tests in the class GstWebAudioPlayerTestCommon

    // GstWebAudioPlayerTestCommon::expectInitAppSrc()
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(&m_appSrc, 10 * 1024));
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
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("appsrc"), StrEq("audsrc")))
        .WillOnce(Return(GST_ELEMENT(&m_appSrc)));

    // From GstWebAudioPlayerTestCommon::expectLinkElements()
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("audioconvert"), _)).WillOnce(Return(&m_convert));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("audioresample"), _)).WillOnce(Return(&m_resample));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), GST_ELEMENT(&m_appSrc))).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_convert)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_resample)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_pipeline), &m_sink)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(GST_ELEMENT(&m_appSrc), &m_convert)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&m_convert, &m_resample)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLink(&m_resample, &m_sink)).WillOnce(Return(TRUE));

    // Similar to GenericTasksTestsBase::shouldAttachAudioSourceWithChannelsAndRate()
    EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq(m_kAudioMimeType))).WillOnce(Return(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("channels"), G_TYPE_INT, m_kPcmChannels));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_gstCaps1, StrEq("rate"), G_TYPE_INT, m_kPcmRate));

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
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(&m_appSrc)).WillOnce(Return(&m_gstCaps2));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_gstCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_gstCaps2));

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus)).Times(2);
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
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcEndOfStream(&m_appSrc)).WillOnce(Return(GST_FLOW_OK));
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
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCurrentLevelBytes(&m_appSrc)).WillOnce(Return(m_kTestDelayFrames * 4));
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
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCurrentLevelBytes(&m_appSrc)).WillOnce(Return(m_kTestDelayFrames * 4));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, m_kTestBufLen * 4, nullptr)).WillOnce(Return(&m_buffer));

    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&m_buffer, 0, _, m_kTestBufLen * 4)).WillOnce(Return(m_kTestBufLen * 4));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_appSrc, &m_buffer)).WillOnce(Return(GST_FLOW_OK));
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

void WebAudioTest::sendStateChanged(GstState oldState, GstState newState, GstState pendingState)
{
    EXPECT_CALL(*m_gstWrapperMock, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
    if (!expectedMessage)
    {
        expectedMessage = make_unique<ExpectMessage<WebAudioPlayerStateEvent>>(m_clientStub);
    }
    m_gstreamerStub.sendStateChanged(oldState, newState, pendingState, true);
}

void WebAudioTest::checkMessageReceivedForStateChange(firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState expect)
{
    auto message = expectedMessage->getMessage();
    ASSERT_TRUE(message);
    ASSERT_EQ(message->web_audio_player_handle(), m_webAudioPlayerHandle);
    ASSERT_EQ(message->state(), expect);

    // There shouldn't be a subsequent message...
    expectedMessage = make_unique<ExpectMessage<WebAudioPlayerStateEvent>>(m_clientStub);
    message = expectedMessage->getMessage();
    ASSERT_FALSE(message);
}

void WebAudioTest::willGstSendEos()
{
    EXPECT_CALL(*m_gstWrapperMock, gstEventNewFlushStart()).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstEventNewFlushStop(_)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSendEvent(_, _)).WillRepeatedly(Return(true));
}

void WebAudioTest::gstSendEos()
{
    m_gstreamerStub.sendEos();
}

/*
 * Component Test: Web Audio Player Playback
 * Test Objective:
 *  Test the playback of web audio player.
 *
 * Sequence Diagrams:
 *  https://wiki.rdkcentral.com/display/ASP/Rialto+Web+Audio
 *  Initialisation & Termination
 *  GetBufferAvailable, WriteBuffer, Pause, Play, SetEOS
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: WebAudioPlayer
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *
 * Test Steps:
 *  Step 1: Create a new web audio player
 *   Create an instance of WebAudioPlayer.
 *   Expect that web audio api is called on the server
 *   Check that the object returned is valid.
 *   Check that web audio player has been added.
 *
 *  Step 2: Get the available buffer
 *   getBufferAvailable.
 *   Expect that getBufferAvailable is propagated to the server.
 *   Api call return the available frames and web audio shm info.
 *   Check available frames and web audio shm info.
 *
 *  Step 3: Get the write buffer
 *   writeBuffer.
 *   Expect that writeBuffer is propagated to the server.
 *   Api call return the web audio session, number of frames and data.
 *   Check web audio session, number of frames and data.
 *
 *  Step 4: Pause
 *   pause().
 *   Expect that pause is propagated to the server.
 *   Api call return the status.
 *   Check status is paused.
 *
 *  Step 5: Notify state to PAUSE
 *   WebAudioPlayerStateChange to PAUSE
 *
 *  Step 6: Play
 *   play().
 *   Expect that play is propagated to the server.
 *   Api call return the status.
 *   Check status is play.
 *
 *  Step 7: Notify state to PLAY
 *   WebAudioPlayerStateChange to PLAY
 *
 *  Step 8: Get the available buffer
 *   getBufferAvailable.
 *   Expect that getBufferAvailable is propagated to the server.
 *   Api call return the available frames and web audio shm info.
 *   Check available frames and web audio shm info.
 *
 *  Step 9: Get the write buffer
 *   writeBuffer.
 *   Expect that writeBuffer is propagated to the server.
 *   Api call return the web audio session, number of frames and data.
 *   Check web audio session, number of frames and data.
 *
 *  Step 10: Set end of stream
 *   setEos.
 *   Expect that setEos is propagated to the server.
 *   Api call returns a status of true.
 *   Check return status is true.
 *
 *  Step 11: Notify state to EOS
 *   WebAudioPlayerStateChange to EOS
 *
 *  Step 12: Destroy web audio player session
 *   Destroy instance of WebAudioPlayer.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  To create a web audio player gracefully.
 *  To get the available buffer and write to the buffer correctly.
 *  To pause, play and set end of stream successfully.
 *
 * Code:
 */
TEST_F(WebAudioTest, testAllApisWithMultipleQueries)
{
    // step 1: Create a new web audio player
    willCreateWebAudioPlayer();
    createWebAudioPlayer();
    sendStateChanged(GST_STATE_NULL, GST_STATE_READY, GST_STATE_NULL);
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_IDLE);

    // Step 2: Get the available buffer
    webAudioGetBufferAvailable();

    // Step 3: Get the write buffer
    willWebAudioWriteBuffer();
    webAudioWriteBuffer();

    // Step 6: Play
    willWebAudioPlay();
    webAudioPlay();
    sendStateChanged(GST_STATE_NULL, GST_STATE_PLAYING, GST_STATE_NULL);
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_PLAYING);

    // Step 4: Pause
    // ExpectMessage<WebAudioPlayerStateEvent> expectedMessage(m_clientStub);
    willWebAudioPause();
    webAudioPause();
    sendStateChanged(GST_STATE_PLAYING, GST_STATE_PAUSED, GST_STATE_NULL);
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_PAUSED);

    willWebAudioGetBufferDelay();
    webAudioGetBufferDelay();

    webAudioGetDeviceInfo();

    willWebAudioSetVolume();
    webAudioSetVolume();

    willWebAudioGetVolume();
    webAudioGetVolume();

    willWebAudioSetEos();
    webAudioSetEos();
    willGstSendEos();
    gstSendEos();
    checkMessageReceivedForStateChange(WebAudioPlayerStateEvent_WebAudioPlayerState_END_OF_STREAM);

    destroyWebAudioPlayer();
}

/*
 * Component Test: Play and Pause Api Failures
 * Test Objective:
 *  Check that failures returned directly from the Play and Pause apis and failures returned asyncronously
 *  during server WebAudioPlayer state changes are handled correctly. Subsequent Api requests after failures are successful.
 *
 * Sequence Diagrams:
 *  Play and Pause Api Failures -> https://wiki.rdkcentral.com/display/ASP/Rialto+Web+Audio
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: WebAudioPlayer
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *
 * Test Steps:
 *
 *  Step 1: Create a new web audio player session
 *   Create an instance of WebAudioPlayer.
 *   Expect that web audio api is called on the server
 *   Check that the object returned is valid.
 *   Check that web audio player has been added.
 *
 *  Step 2: Play api failure
 *   Play the content.
 *   Expect that play propagated to the server and return failure.
 *
 *  Step 3: Playing state failure
 *   play().
 *   Expect that play propagated to the server.
 *   Server notifies the client that the WebAudioPlayer state has changed to FAILURE.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 4: Notify state to FAILURE
 *   WebAudioPlayerStateChange to FAILURE
 *
 *  Step 5: Play
 *   play().
 *   Expect that play is propagated to the server.
 *   Api call return the status.
 *   Check status is play.
 *
 *  Step 6: Notify state to PLAY
 *   WebAudioPlayerStateChange to PLAY
 *
 *  Step 7: Pause api failure
 *   Pause the content.
 *   Expect that pause propagated to the server and return failure.
 *
 *  Step 8: Pause state failure
 *   Pause().
 *   Expect that pause propagated to the server.
 *   Server notifies the client that the WebAudioPlayer state has changed to FAILURE.
 *   Expect that the state change notification is propagated to the client.
 *
 *   Step 9: Notify state to FAILURE
 *   WebAudioPlayerStateChange to FAILURE
 *
 *  Step 10: Pause
 *   pause().
 *   Expect that pause is propagated to the server.
 *   Api call return the status.
 *   Check status is paused.
 *
 *  Step 11: Notify state to PAUSE
 *   WebAudioPlayerStateChange to PAUSE
 *
 *  Step 12: Destroy web audio player session
 *   Destroy instance of WebAudioPlayer.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  All failures are notified to the calling application.
 *  Failures are recoverable.
 *
 * Code:
 */
TEST_F(WebAudioTest, testAllApisWithMultipleQueries2)
{
    // todo failures
}

} // namespace firebolt::rialto::server::ct
