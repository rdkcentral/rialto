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

#include "WebAudioPlayerModuleServiceTestsFixture.h"
#include "WebAudioPlayerModuleService.h"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <vector>

using testing::_;
using testing::ByRef;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SetArgReferee;

namespace
{
constexpr int webAudioPlayerHandle{0};
const std::string audioMimeType{"audio/x-raw"};
constexpr uint32_t priority{4};
constexpr firebolt::rialto::WebAudioPcmConfig pcmConfig{1, 2, 3, false, true, false};
constexpr firebolt::rialto::WebAudioPlayerState webAudioPlayerState{firebolt::rialto::WebAudioPlayerState::END_OF_STREAM};
constexpr uint32_t availableFrames = 11;
const firebolt::rialto::WebAudioShmInfo shmInfo{12,13,14,15};
constexpr uint32_t delayFrames = 16;
constexpr uint32_t numberOfFrames = 17;
constexpr uint32_t preferredFrames = 18;
constexpr uint32_t maximumFrames = 19;
constexpr bool supportDeferredPlay = true;
constexpr double volume = 1.5;
} // namespace

MATCHER_P(WebAudioPlayerStateEventMatcher, playerState, "")
{
    std::shared_ptr<firebolt::rialto::WebAudioPlayerStateEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::WebAudioPlayerStateEvent>(arg);
    return (playerState == event->state());
}

MATCHER_P(PcmConfigMatcher, expectedPcmConfig, "")
{
    firebolt::rialto::WebAudioPcmConfig actualPcmConfig = arg->pcm;
    return ((actualPcmConfig.rate == expectedPcmConfig.rate) &&
            (actualPcmConfig.channels == expectedPcmConfig.channels) &&
            (actualPcmConfig.sampleSize == expectedPcmConfig.sampleSize) &&
            (actualPcmConfig.isBigEndian == expectedPcmConfig.isBigEndian) &&
            (actualPcmConfig.isSigned == expectedPcmConfig.isSigned) &&
            (actualPcmConfig.isFloat == expectedPcmConfig.isFloat));
}

namespace firebolt::rialto
{
firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState convertWebAudioPlayerState(const firebolt::rialto::WebAudioPlayerState &mediaType)
{
    switch (mediaType)
    {
    case firebolt::rialto::WebAudioPlayerState::UNKNOWN:
    {
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState::WebAudioPlayerStateEvent_WebAudioPlayerState_UNKNOWN;
    }
    case firebolt::rialto::WebAudioPlayerState::IDLE:
    {
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState::WebAudioPlayerStateEvent_WebAudioPlayerState_IDLE;
    }
    case firebolt::rialto::WebAudioPlayerState::PLAYING:
    {
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState::WebAudioPlayerStateEvent_WebAudioPlayerState_PLAYING;
    }
    case firebolt::rialto::WebAudioPlayerState::PAUSED:
    {
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState::WebAudioPlayerStateEvent_WebAudioPlayerState_PAUSED;
    }
    case firebolt::rialto::WebAudioPlayerState::END_OF_STREAM:
    {
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState::WebAudioPlayerStateEvent_WebAudioPlayerState_END_OF_STREAM;
    }
    case firebolt::rialto::WebAudioPlayerState::FAILURE:
    {
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState::WebAudioPlayerStateEvent_WebAudioPlayerState_FAILURE;
    }
    }
    return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState::WebAudioPlayerStateEvent_WebAudioPlayerState_UNKNOWN;
}
} // namespace firebolt::rialto

WebAudioPlayerModuleServiceTests::WebAudioPlayerModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()},
      m_shmInfo{std::make_shared<firebolt::rialto::WebAudioShmInfo>(shmInfo)}
{
    m_service = std::make_shared<firebolt::rialto::server::ipc::WebAudioPlayerModuleService>(m_webAudioPlayerServiceMock);
}

WebAudioPlayerModuleServiceTests::~WebAudioPlayerModuleServiceTests() {}

void WebAudioPlayerModuleServiceTests::clientWillConnect()
{
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void WebAudioPlayerModuleServiceTests::clientWillDisconnect(int handle)
{
    EXPECT_CALL(m_webAudioPlayerServiceMock, destroyWebAudioPlayer(handle)).WillOnce(Return(true));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillCreateWebAudioPlayer()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).Times(2).WillRepeatedly(Return(m_clientMock));
    EXPECT_CALL(m_webAudioPlayerServiceMock, createWebAudioPlayer(_, _, audioMimeType, priority, _))
        .WillOnce(DoAll(SaveArg<1>(&m_webAudioPlayerClient), Return(true)));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillCreateWebAudioPlayerWithPcmConfig()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).Times(2).WillRepeatedly(Return(m_clientMock));
    EXPECT_CALL(m_webAudioPlayerServiceMock, createWebAudioPlayer(_, _, audioMimeType, priority, PcmConfigMatcher(pcmConfig)))
        .WillOnce(DoAll(SaveArg<1>(&m_webAudioPlayerClient), Return(true)));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToCreateWebAudioPlayer()
{
    expectRequestFailure();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_webAudioPlayerServiceMock, createWebAudioPlayer(_, _, audioMimeType, priority, _)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillDestroyWebAudioPlayer()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_webAudioPlayerServiceMock, destroyWebAudioPlayer(webAudioPlayerHandle)).WillOnce(Return(true));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToDestroyWebAudioPlayer()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, destroyWebAudioPlayer(webAudioPlayerHandle)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillPlay()
{
    expectRequestSuccess();
    EXPECT_CALL(m_webAudioPlayerServiceMock, play(webAudioPlayerHandle)).WillOnce(Return(true));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToPlay()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, play(webAudioPlayerHandle)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillPause()
{
    expectRequestSuccess();
    EXPECT_CALL(m_webAudioPlayerServiceMock, pause(webAudioPlayerHandle)).WillOnce(Return(true));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToPause()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, pause(webAudioPlayerHandle)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillSetEos()
{
    expectRequestSuccess();
    EXPECT_CALL(m_webAudioPlayerServiceMock, setEos(webAudioPlayerHandle)).WillOnce(Return(true));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToSetEos()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, setEos(webAudioPlayerHandle)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillGetBufferAvailable()
{
    expectRequestSuccess();
    EXPECT_CALL(m_webAudioPlayerServiceMock, getBufferAvailable(webAudioPlayerHandle, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(availableFrames), SetArgReferee<2>(m_shmInfo), Return(true)));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToGetBufferAvailable()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, getBufferAvailable(webAudioPlayerHandle, _, _)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillGetBufferDelay()
{
    expectRequestSuccess();
    EXPECT_CALL(m_webAudioPlayerServiceMock, getBufferDelay(webAudioPlayerHandle, _))
        .WillOnce(DoAll(SetArgReferee<1>(delayFrames), Return(true)));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToGetBufferDelay()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, getBufferDelay(webAudioPlayerHandle, _)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillWriteBuffer()
{
    expectRequestSuccess();
    EXPECT_CALL(m_webAudioPlayerServiceMock, writeBuffer(webAudioPlayerHandle, numberOfFrames, _)).WillOnce(Return(true));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToWriteBuffer()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, writeBuffer(webAudioPlayerHandle, numberOfFrames, _)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillGetDeviceInfo()
{
    expectRequestSuccess();
    EXPECT_CALL(m_webAudioPlayerServiceMock, getDeviceInfo(webAudioPlayerHandle, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(preferredFrames), SetArgReferee<2>(maximumFrames), SetArgReferee<3>(supportDeferredPlay), Return(true)));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToGetDeviceInfo()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, getDeviceInfo(webAudioPlayerHandle, _, _, _)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillSetVolume()
{
    expectRequestSuccess();
    EXPECT_CALL(m_webAudioPlayerServiceMock, setVolume(webAudioPlayerHandle, volume)).WillOnce(Return(true));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToSetVolume()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, setVolume(webAudioPlayerHandle, volume)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillGetVolume()
{
    expectRequestSuccess();
    EXPECT_CALL(m_webAudioPlayerServiceMock, getVolume(webAudioPlayerHandle, _))
        .WillOnce(DoAll(SetArgReferee<1>(volume), Return(true)));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerServiceWillFailToGetVolume()
{
    expectRequestFailure();
    EXPECT_CALL(m_webAudioPlayerServiceMock, getVolume(webAudioPlayerHandle, _)).WillOnce(Return(false));
}

void WebAudioPlayerModuleServiceTests::webAudioPlayerClientWillSendPlayerStateEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(WebAudioPlayerStateEventMatcher(convertWebAudioPlayerState(webAudioPlayerState))));
}

void WebAudioPlayerModuleServiceTests::sendClientConnected()
{
    m_service->clientConnected(m_clientMock);
}

void WebAudioPlayerModuleServiceTests::sendClientDisconnected()
{
    m_service->clientDisconnected(m_clientMock);
}

int WebAudioPlayerModuleServiceTests::sendCreateWebAudioPlayerRequestAndReceiveResponse()
{
    firebolt::rialto::CreateWebAudioPlayerRequest request;
    firebolt::rialto::CreateWebAudioPlayerResponse response;

    // Set an invalid handle in the response
    response.set_web_audio_player_handle(-1);

    request.set_audio_mime_type(audioMimeType);
    request.set_priority(priority);

    m_service->createWebAudioPlayer(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_GE(response.web_audio_player_handle(), 0);

    return response.web_audio_player_handle();
}

int WebAudioPlayerModuleServiceTests::sendCreateWebAudioPlayerRequestWithPcmConfigAndReceiveResponse()
{
    firebolt::rialto::CreateWebAudioPlayerRequest request;
    firebolt::rialto::CreateWebAudioPlayerResponse response;

    // Set an invalid handle in the response
    response.set_web_audio_player_handle(-1);

    request.set_audio_mime_type(audioMimeType);
    request.set_priority(priority);

    firebolt::rialto::CreateWebAudioPlayerRequest_WebAudioConfig* configProto = request.mutable_config();
    firebolt::rialto::CreateWebAudioPlayerRequest_WebAudioPcmConfig* pcmConfigProto = configProto->mutable_pcm();
    pcmConfigProto->set_rate(pcmConfig.rate);
    pcmConfigProto->set_channels(pcmConfig.channels);
    pcmConfigProto->set_sample_size(pcmConfig.sampleSize);
    pcmConfigProto->set_is_big_endian(pcmConfig.isBigEndian);
    pcmConfigProto->set_is_signed(pcmConfig.isSigned);
    pcmConfigProto->set_is_float(pcmConfig.isFloat);

    m_service->createWebAudioPlayer(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_GE(response.web_audio_player_handle(), 0);

    return response.web_audio_player_handle();
}

void WebAudioPlayerModuleServiceTests::sendCreateWebAudioPlayerRequestAndExpectFailure()
{
    firebolt::rialto::CreateWebAudioPlayerRequest request;
    firebolt::rialto::CreateWebAudioPlayerResponse response;

    request.set_audio_mime_type(audioMimeType);
    request.set_priority(priority);

    m_service->createWebAudioPlayer(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendDestroyWebAudioPlayerRequestAndReceiveResponse()
{
    firebolt::rialto::DestroyWebAudioPlayerRequest request;
    firebolt::rialto::DestroyWebAudioPlayerResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->destroyWebAudioPlayer(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendPlayRequestAndReceiveResponse()
{
    firebolt::rialto::WebAudioPlayRequest request;
    firebolt::rialto::WebAudioPlayResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->play(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendPauseRequestAndReceiveResponse()
{
    firebolt::rialto::WebAudioPauseRequest request;
    firebolt::rialto::WebAudioPauseResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->pause(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendSetEosRequestAndReceiveResponse()
{
    firebolt::rialto::WebAudioSetEosRequest request;
    firebolt::rialto::WebAudioSetEosResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->setEos(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendGetBufferAvailableRequestAndReceiveResponse()
{
    firebolt::rialto::WebAudioGetBufferAvailableRequest request;
    firebolt::rialto::WebAudioGetBufferAvailableResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->getBufferAvailable(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.available_frames(), availableFrames);
    EXPECT_EQ(response.shm_info().offset_main(), shmInfo.offsetMain);
    EXPECT_EQ(response.shm_info().length_main(), shmInfo.lengthMain);
    EXPECT_EQ(response.shm_info().offset_wrap(), shmInfo.offsetWrap);
    EXPECT_EQ(response.shm_info().length_wrap(), shmInfo.lengthWrap);
}

void WebAudioPlayerModuleServiceTests::sendGetBufferAvailableRequestAndExpectFailure()
{
    firebolt::rialto::WebAudioGetBufferAvailableRequest request;
    firebolt::rialto::WebAudioGetBufferAvailableResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->getBufferAvailable(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendGetBufferDelayRequestAndReceiveResponse()
{
    firebolt::rialto::WebAudioGetBufferDelayRequest request;
    firebolt::rialto::WebAudioGetBufferDelayResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->getBufferDelay(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.delay_frames(), delayFrames);
}

void WebAudioPlayerModuleServiceTests::sendGetBufferDelayRequestAndExpectFailure()
{
    firebolt::rialto::WebAudioGetBufferDelayRequest request;
    firebolt::rialto::WebAudioGetBufferDelayResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->getBufferDelay(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendWriteBufferRequestAndReceiveResponse()
{
    firebolt::rialto::WebAudioWriteBufferRequest request;
    firebolt::rialto::WebAudioWriteBufferResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);
    request.set_number_of_frames(numberOfFrames);

    m_service->writeBuffer(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendGetDeviceInfoRequestAndReceiveResponse()
{
    firebolt::rialto::WebAudioGetDeviceInfoRequest request;
    firebolt::rialto::WebAudioGetDeviceInfoResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->getDeviceInfo(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.preferred_frames(), preferredFrames);
    EXPECT_EQ(response.maximum_frames(), maximumFrames);
    EXPECT_EQ(response.support_deferred_play(), supportDeferredPlay);
}

void WebAudioPlayerModuleServiceTests::sendGetDeviceInfoRequestAndExpectFailure()
{
    firebolt::rialto::WebAudioGetDeviceInfoRequest request;
    firebolt::rialto::WebAudioGetDeviceInfoResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->getDeviceInfo(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendSetVolumeRequestAndReceiveResponse()
{
    firebolt::rialto::WebAudioSetVolumeRequest request;
    firebolt::rialto::WebAudioSetVolumeResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);
    request.set_volume(volume);

    m_service->setVolume(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendGetVolumeRequestAndReceiveResponse()
{
    firebolt::rialto::WebAudioGetVolumeRequest request;
    firebolt::rialto::WebAudioGetVolumeResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->getVolume(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.volume(), volume);
}

void WebAudioPlayerModuleServiceTests::sendGetVolumeRequestAndExpectFailure()
{
    firebolt::rialto::WebAudioGetVolumeRequest request;
    firebolt::rialto::WebAudioGetVolumeResponse response;

    request.set_web_audio_player_handle(webAudioPlayerHandle);

    m_service->getVolume(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void WebAudioPlayerModuleServiceTests::sendPlayerStateEvent()
{
    ASSERT_TRUE(m_webAudioPlayerClient);
    m_webAudioPlayerClient->notifyState(webAudioPlayerState);
}

void WebAudioPlayerModuleServiceTests::expectRequestSuccess()
{
    EXPECT_CALL(*m_closureMock, Run());
}

void WebAudioPlayerModuleServiceTests::expectRequestFailure()
{
    EXPECT_CALL(*m_controllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}
