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

#ifndef FIREBOLT_RIALTO_SERVER_CT_WEB_AUDIO_TEST_METHODS_H_
#define FIREBOLT_RIALTO_SERVER_CT_WEB_AUDIO_TEST_METHODS_H_

#include <memory>
#include <queue>

#include "ExpectMessage.h"
#include "GstSrc.h"
#include "GstreamerStub.h"
#include "RialtoServerComponentTest.h"
#include "ShmHandle.h"
#include "webaudioplayermodule.pb.h"

namespace firebolt::rialto::server::ct
{
class WebAudioTestMethods : public RialtoServerComponentTest
{
public:
    WebAudioTestMethods();
    virtual ~WebAudioTestMethods();

    void willCreateWebAudioPlayer();
    void createWebAudioPlayer();

    void willFailToCreateWebAudioPlayer();
    void failToCreateWebAudioPlayer();

    void willWebAudioPlay();
    void willWebAudioPlayFail();
    void webAudioPlay();

    void willWebAudioPause();
    void willWebAudioPauseFail();
    void webAudioPause();

    void willWebAudioSetEos();
    void webAudioSetEos();

    void webAudioGetBufferAvailable(int &offsetMain, int &lengthMain, int &offsetWrap, int &lengthWrap);

    void willWebAudioWriteBuffer(int len);
    void webAudioWriteBuffer(int len);

    void willWebAudioGetBufferDelay();
    void webAudioGetBufferDelay();

    void webAudioGetDeviceInfo();

    void willWebAudioSetVolume();
    void webAudioSetVolume();

    void willWebAudioGetVolume();
    void webAudioGetVolume();

    void destroyWebAudioPlayer();

    //////////////////////////////////////////////
    // This section is for shared memory testing
    void initShm();
    int checkInitialBufferAvailable();
    int getBufferAvailable();
    void sendDataToShm(int len);
    //////////////////////////////////////////////

protected:
    void sendStateChanged(GstState oldState, GstState newState, GstState pendingState);
    void checkMessageReceivedForStateChange(firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState expect);

    void willGstSendEos();
    void gstSendEos();

    std::unique_ptr<ExpectMessage<WebAudioPlayerStateEvent>> m_expectedMessage;

    //////////////////////////////////////////////
    // This section is for shared memory testing
    ShmHandle m_shmHandle;
    std::queue<std::uint8_t> m_dataFifo;
    //////////////////////////////////////////////

    int m_webAudioPlayerHandle{-1};

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
    GstElement m_volume;
    GstElement m_queue;
    GstBuffer m_buffer;
    gchar m_capsStr;

    GstBin m_rialtoSrcBin = {};
    GstRialtoSrcPrivate m_rialtoSrcPriv = {};
    GstRialtoSrc m_rialtoSource = {m_rialtoSrcBin, &m_rialtoSrcPriv};
    GstreamerStub m_gstreamerStub{m_glibWrapperMock, m_gstWrapperMock, &m_pipeline, &m_bus, GST_ELEMENT(&m_rialtoSource)};
};

} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_WEB_AUDIO_TEST_METHODS_H_
