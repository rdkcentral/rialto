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

#include "TasksTestsBase.h"

class RemoveSourceTest : public TasksTestsBase
{
protected:

    RemoveSourceTest()
    {
        setContextNeedDataAudioOnly();
        setContextAudioBuffer();
        setContextNeedDataPendingAudioOnly(true);
        setContextAudioSourceRemoved();
        setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    }
};

TEST_F(RemoveSourceTest, shouldRemoveAudioSourceWithoutFlushing)
{
    setContextStreamInfoEmpty();
    shouldInvalidateActiveAudioRequests();
    triggerRemoveSourceAudio();
    checkNoMoreNeedData();
    checkNoNeedDataPendingForBothSources();
    checkAudioSourceRemoved();
    checkBuffersEmpty();
}
#if 0
TEST_F(RemoveSourceTest, shouldNotRemoveVideoSource)
{
    constexpr auto kMediaSourceType{firebolt::rialto::MediaSourceType::VIDEO};
    firebolt::rialto::server::tasks::generic::RemoveSource task{m_context, &m_gstPlayerClient, m_gstWrapper,
                                                                kMediaSourceType};
    task.execute();
    EXPECT_TRUE(m_context.audioNeedData);
    EXPECT_TRUE(m_context.audioNeedDataPending);
    EXPECT_FALSE(m_context.audioSourceRemoved);
}

TEST_F(RemoveSourceTest, shouldRemoveAudioSource)
{
    constexpr auto kMediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
    EXPECT_CALL(m_gstPlayerClient, invalidateActiveRequests(kMediaSourceType));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&m_flushStartEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_audioSrc, &m_flushStartEvent)).WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStop(FALSE)).WillOnce(Return(&m_flushStopEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_audioSrc, &m_flushStopEvent)).WillOnce(Return(TRUE));
    firebolt::rialto::server::tasks::generic::RemoveSource task{m_context, &m_gstPlayerClient, m_gstWrapper,
                                                                kMediaSourceType};
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.audioSourceRemoved);
    EXPECT_TRUE(m_context.audioBuffers.empty());
}

TEST_F(RemoveSourceTest, shouldRemoveAudioSourceFlushEventError)
{
    constexpr auto kMediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
    EXPECT_CALL(m_gstPlayerClient, invalidateActiveRequests(kMediaSourceType));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&m_flushStartEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_audioSrc, &m_flushStartEvent)).WillOnce(Return(FALSE));
    EXPECT_CALL(*m_gstWrapper, gstEventNewFlushStop(FALSE)).WillOnce(Return(&m_flushStopEvent));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_audioSrc, &m_flushStopEvent)).WillOnce(Return(FALSE));
    firebolt::rialto::server::tasks::generic::RemoveSource task{m_context, &m_gstPlayerClient, m_gstWrapper,
                                                                kMediaSourceType};
    task.execute();
    EXPECT_FALSE(m_context.audioNeedData);
    EXPECT_FALSE(m_context.audioNeedDataPending);
    EXPECT_TRUE(m_context.audioSourceRemoved);
}
#endif