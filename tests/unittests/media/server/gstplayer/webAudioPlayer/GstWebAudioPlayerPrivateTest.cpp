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

class GstWebAudioPlayerPrivateTest : public GstWebAudioPlayerTestCommon
{
protected:
    std::unique_ptr<IGstWebAudioPlayerPrivate> m_sut;

    GstWebAudioPlayerPrivateTest()
    {
        gstPlayerWillBeCreatedForGenericPlatform();
        m_sut = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock, m_glibWrapperMock,
                                                    m_gstSrcFactoryMock, std::move(m_taskFactory),
                                                    std::move(workerThreadFactory),
                                                    std::move(gstDispatcherThreadFactory));
    }

    ~GstWebAudioPlayerPrivateTest() override
    {
        gstPlayerWillBeDestroyed();
        m_sut.reset();
    }
};

TEST_F(GstWebAudioPlayerPrivateTest, shouldFailToChangePlaybackStateWhenSetStateFails)
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(_, GST_STATE_PLAYING)).WillOnce(Return(GST_STATE_CHANGE_FAILURE));
    EXPECT_CALL(m_gstPlayerClient, notifyState(WebAudioPlayerState::FAILURE));
    EXPECT_FALSE(m_sut->changePipelineState(GST_STATE_PLAYING));
}

TEST_F(GstWebAudioPlayerPrivateTest, shouldChangePlaybackState)
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(_, GST_STATE_PLAYING)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_TRUE(m_sut->changePipelineState(GST_STATE_PLAYING));
}

TEST_F(GstWebAudioPlayerPrivateTest, shouldStopWorkerThread)
{
    EXPECT_CALL(m_workerThreadMock, stop());
    m_sut->stopWorkerThread();
}
