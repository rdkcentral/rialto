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

#include "tasks/webAudio/WebAudioPlayerTaskFactory.h"
#include "WebAudioPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstWebAudioPlayerClientMock.h"
#include "GstWebAudioPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "tasks/IPlayerTask.h"
#include "tasks/webAudio/Shutdown.h"
#include "tasks/webAudio/Stop.h"
#include "tasks/webAudio/Play.h"
#include "tasks/webAudio/Pause.h"
#include "tasks/webAudio/Eos.h"
#include "tasks/webAudio/SetVolume.h"
#include "tasks/webAudio/SetCaps.h"
#include "tasks/webAudio/WriteBuffer.h"
#include "tasks/webAudio/HandleBusMessage.h"
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrictMock;

class WebAudioPlayerTaskFactoryTest : public testing::Test
{
protected:
    firebolt::rialto::server::WebAudioPlayerContext m_context;
    StrictMock<firebolt::rialto::server::GstWebAudioPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstWebAudioPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    firebolt::rialto::server::WebAudioPlayerTaskFactory m_sut{&m_gstPlayerClient, m_gstWrapper, m_glibWrapper};
};

TEST_F(WebAudioPlayerTaskFactoryTest, ShouldCreateSetCaps)
{
    const std::string audioMimeType{"audio/x-raw"};
    const firebolt::rialto::WebAudioConfig config{};
    auto task = m_sut.createSetCaps(m_context, audioMimeType, &config);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::webaudio::SetCaps &>(*task));
}

TEST_F(WebAudioPlayerTaskFactoryTest, ShouldCreatePlay)
{
    auto task = m_sut.createPlay(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::webaudio::Play &>(*task));
}

TEST_F(WebAudioPlayerTaskFactoryTest, ShouldCreatePause)
{
    auto task = m_sut.createPause(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::webaudio::Pause &>(*task));
}

TEST_F(WebAudioPlayerTaskFactoryTest, ShouldCreateEos)
{
    auto task = m_sut.createEos(m_context);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::webaudio::Eos &>(*task));
}

TEST_F(WebAudioPlayerTaskFactoryTest, ShouldStop)
{
    auto task = m_sut.createStop(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::webaudio::Stop &>(*task));
}

TEST_F(WebAudioPlayerTaskFactoryTest, ShouldShutdown)
{
    auto task = m_sut.createShutdown(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::webaudio::Shutdown &>(*task));
}

TEST_F(WebAudioPlayerTaskFactoryTest, ShouldSetVolume)
{
    auto task = m_sut.createSetVolume(m_context, {});
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::webaudio::SetVolume &>(*task));
}

TEST_F(WebAudioPlayerTaskFactoryTest, ShouldWriteBuffer)
{
    uint8_t mainPtr{};
    uint8_t wrapPtr{};
    auto task = m_sut.createWriteBuffer(m_context, m_gstPlayer, &mainPtr, {}, &wrapPtr, {});
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::webaudio::WriteBuffer &>(*task));
}

TEST_F(WebAudioPlayerTaskFactoryTest, ShouldHandleBusMessage)
{
    GstMessage message{};
    auto task = m_sut.createHandleBusMessage(m_context, m_gstPlayer, &message);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::webaudio::HandleBusMessage &>(*task));
}
