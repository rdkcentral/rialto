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

#include "GstWebAudioPlayerTestCommon.h"
#include "IGstDispatcherThreadClient.h"
#include "PlayerTaskMock.h"

using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Ref;
using testing::Return;

class WebAudioGstDispatcherThreadClientTest : public GstWebAudioPlayerTestCommon
{
protected:
    std::unique_ptr<IGstDispatcherThreadClient> m_sut;

    WebAudioGstDispatcherThreadClientTest()
    {
        gstPlayerWillBeCreatedForGenericPlatform();
        m_sut = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock, m_glibWrapperMock,
                                                    m_gstSrcFactoryMock, std::move(m_taskFactory),
                                                    std::move(workerThreadFactory),
                                                    std::move(gstDispatcherThreadFactory));
    }

    ~WebAudioGstDispatcherThreadClientTest() override
    {
        gstPlayerWillBeDestroyed();
        m_sut.reset();
    }
};

TEST_F(WebAudioGstDispatcherThreadClientTest, shouldHandleBusMessage)
{
    GstMessage message{};
    std::unique_ptr<IPlayerTask> messageTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*messageTask), execute());
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillRepeatedly(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
    EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, &message)).WillOnce(Return(ByMove(std::move(messageTask))));

    m_sut->handleBusMessage(&message);
}
