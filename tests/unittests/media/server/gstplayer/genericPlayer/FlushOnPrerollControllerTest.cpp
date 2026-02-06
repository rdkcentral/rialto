/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "FlushOnPrerollController.h"
#include <gtest/gtest.h>
#include <thread>

using firebolt::rialto::MediaSourceType;
using firebolt::rialto::server::FlushOnPrerollController;

class FlushOnPrerollControllerTest : public ::testing::Test
{
protected:
    FlushOnPrerollController m_sut;
};

TEST_F(FlushOnPrerollControllerTest, shouldNotWaithWhenNoFlushSet)
{
    m_sut.waitIfRequired(MediaSourceType::AUDIO);
    // No deadlock here
}

TEST_F(FlushOnPrerollControllerTest, shouldNotWaitWhenNotPrerolled)
{
    m_sut.setTargetState(GST_STATE_PLAYING);
    m_sut.setFlushing(MediaSourceType::AUDIO);
    m_sut.waitIfRequired(MediaSourceType::AUDIO);
    // No deadlock here
}

TEST_F(FlushOnPrerollControllerTest, shouldNotWaitWhenReset)
{
    m_sut.setTargetState(GST_STATE_PLAYING);
    m_sut.setFlushing(MediaSourceType::AUDIO);
    m_sut.stateReached(GST_STATE_PAUSED);
    m_sut.reset();
    m_sut.waitIfRequired(MediaSourceType::AUDIO);
    // No deadlock here
}

TEST_F(FlushOnPrerollControllerTest, shouldNotWaitWhenPreviousProcedureIsFinished)
{
    m_sut.setTargetState(GST_STATE_PLAYING);
    m_sut.setFlushing(MediaSourceType::AUDIO);
    m_sut.stateReached(GST_STATE_PAUSED);
    m_sut.stateReached(GST_STATE_PLAYING);
    m_sut.waitIfRequired(MediaSourceType::AUDIO);
    // No deadlock here
}

TEST_F(FlushOnPrerollControllerTest, shouldNotWaitWithVideoFlushWhenOnlyAudioIsOngoing)
{
    m_sut.setTargetState(GST_STATE_PLAYING);
    m_sut.setFlushing(MediaSourceType::AUDIO);
    m_sut.stateReached(GST_STATE_PAUSED);
    m_sut.waitIfRequired(MediaSourceType::VIDEO);
    // No deadlock here
}

TEST_F(FlushOnPrerollControllerTest, shouldWaitForAudioFlushFinish)
{
    m_sut.setTargetState(GST_STATE_PLAYING);
    m_sut.setFlushing(MediaSourceType::AUDIO);
    m_sut.stateReached(GST_STATE_PAUSED);
    std::thread waitThread([this]() { m_sut.waitIfRequired(MediaSourceType::AUDIO); });
    m_sut.stateReached(GST_STATE_PLAYING);
    waitThread.join();
    // No deadlock here
}
