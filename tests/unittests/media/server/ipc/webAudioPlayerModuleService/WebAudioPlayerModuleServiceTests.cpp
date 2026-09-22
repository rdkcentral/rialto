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

#include "WebAudioPlayerModuleService.h"
#include "WebAudioPlayerModuleServiceTestsFixture.h"
#include <chrono>
#include <future>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>

using testing::_;
using testing::Return;

TEST_F(WebAudioPlayerModuleServiceTests, shouldConnectClient)
{
    clientWillConnect();
    sendClientConnected();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldCreateWebAudioPlayer)
{
    webAudioPlayerServiceWillCreateWebAudioPlayer();
    sendCreateWebAudioPlayerRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldCreateWebAudioPlayerWithPcmConfig)
{
    webAudioPlayerServiceWillCreateWebAudioPlayerWithPcmConfig();
    sendCreateWebAudioPlayerRequestWithPcmConfigAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToCreateWebAudioPlayer)
{
    webAudioPlayerServiceWillFailToCreateWebAudioPlayer();
    sendCreateWebAudioPlayerRequestAndExpectFailure();
}

TEST(WebAudioPlayerModuleServiceConcurrencyTests, shouldCompleteCreateResponseBeforeDisconnectCleanup)
{
    StrictMock<firebolt::rialto::server::service::WebAudioPlayerServiceMock> webAudioPlayerService;
    auto service = std::make_shared<firebolt::rialto::server::ipc::WebAudioPlayerModuleService>(webAudioPlayerService);
    auto client = std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>();
    StrictMock<firebolt::rialto::ipc::ControllerMock> controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> closure;
    firebolt::rialto::CreateWebAudioPlayerRequest request;
    firebolt::rialto::CreateWebAudioPlayerResponse response;
    std::promise<void> createStarted;
    std::promise<void> allowCreate;
    auto allowCreateFuture = allowCreate.get_future().share();

    EXPECT_CALL(*client, exportService(_));
    service->clientConnected(client);
    EXPECT_CALL(controller, getClient()).Times(2).WillRepeatedly(Return(client));
    EXPECT_CALL(webAudioPlayerService, createWebAudioPlayer(_, _, _, _, _, _, _))
        .WillOnce(testing::Invoke(
            [&](int, const std::shared_ptr<firebolt::rialto::IWebAudioPlayerClient> &, const std::string &, std::uint32_t,
                std::weak_ptr<const firebolt::rialto::WebAudioConfig>, std::int32_t &shmFd, std::uint32_t &shmSize)
            {
                createStarted.set_value();
                allowCreateFuture.wait();
                shmFd = 3;
                shmSize = 1024;
                return true;
            }));
    EXPECT_CALL(closure, Run());
    EXPECT_CALL(webAudioPlayerService, destroyWebAudioPlayer(_)).WillOnce(Return(true));

    std::thread createThread{[&]() { service->createWebAudioPlayer(&controller, &request, &response, &closure); }};
    createStarted.get_future().wait();
    auto disconnectFuture = std::async(std::launch::async, [&]() { service->clientDisconnected(client); });

    EXPECT_EQ(disconnectFuture.wait_for(std::chrono::milliseconds{50}), std::future_status::timeout);
    allowCreate.set_value();
    createThread.join();
    disconnectFuture.get();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldDestroyWebAudioPlayerWhenDisconnectClient)
{
    clientWillConnect();
    sendClientConnected();
    webAudioPlayerServiceWillCreateWebAudioPlayer();
    int handle = sendCreateWebAudioPlayerRequestAndReceiveResponse();
    clientWillDisconnect(handle);
    sendClientDisconnected();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldDestroyWebAudioPlayer)
{
    webAudioPlayerServiceWillDestroyWebAudioPlayer();
    sendDestroyWebAudioPlayerRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToDestroyWebAudioPlayer)
{
    webAudioPlayerServiceWillFailToDestroyWebAudioPlayer();
    sendDestroyWebAudioPlayerRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldPlay)
{
    webAudioPlayerServiceWillPlay();
    sendPlayRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToPlay)
{
    webAudioPlayerServiceWillFailToPlay();
    sendPlayRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldPause)
{
    webAudioPlayerServiceWillPause();
    sendPauseRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToPause)
{
    webAudioPlayerServiceWillFailToPause();
    sendPauseRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldSetEos)
{
    webAudioPlayerServiceWillSetEos();
    sendSetEosRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToSetEos)
{
    webAudioPlayerServiceWillFailToSetEos();
    sendSetEosRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldGetBufferAvailable)
{
    webAudioPlayerServiceWillGetBufferAvailable();
    sendGetBufferAvailableRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToGetBufferAvailable)
{
    webAudioPlayerServiceWillFailToGetBufferAvailable();
    sendGetBufferAvailableRequestAndExpectFailure();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldGetBufferDelay)
{
    webAudioPlayerServiceWillGetBufferDelay();
    sendGetBufferDelayRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToGetBufferDelay)
{
    webAudioPlayerServiceWillFailToGetBufferDelay();
    sendGetBufferDelayRequestAndExpectFailure();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldWriteBuffer)
{
    webAudioPlayerServiceWillWriteBuffer();
    sendWriteBufferRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToWriteBuffer)
{
    webAudioPlayerServiceWillFailToWriteBuffer();
    sendWriteBufferRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldGetDeviceInfo)
{
    webAudioPlayerServiceWillGetDeviceInfo();
    sendGetDeviceInfoRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToGetDeviceInfo)
{
    webAudioPlayerServiceWillFailToGetDeviceInfo();
    sendGetDeviceInfoRequestAndExpectFailure();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldSetVolume)
{
    webAudioPlayerServiceWillSetVolume();
    sendSetVolumeRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToSetVolume)
{
    webAudioPlayerServiceWillFailToSetVolume();
    sendSetVolumeRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldGetVolume)
{
    webAudioPlayerServiceWillGetVolume();
    sendGetVolumeRequestAndReceiveResponse();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldFailToGetVolume)
{
    webAudioPlayerServiceWillFailToGetVolume();
    sendGetVolumeRequestAndExpectFailure();
}

TEST_F(WebAudioPlayerModuleServiceTests, shouldSendPlayerStateEvent)
{
    webAudioPlayerServiceWillCreateWebAudioPlayer();
    sendCreateWebAudioPlayerRequestAndReceiveResponse();
    webAudioPlayerClientWillSendPlayerStateEvent();
    sendPlayerStateEvent();
}

TEST_F(WebAudioPlayerModuleServiceTests, FactoryCreatesObject)
{
    testFactoryCreatesObject();
}
