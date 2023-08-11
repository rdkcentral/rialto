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

#include "WebAudioTasksTestsBase.h"
#include "Matchers.h"
#include "WebAudioTasksTestsContext.h"
#include "tasks/webAudio/Eos.h"
#include "tasks/webAudio/Pause.h"
#include "tasks/webAudio/Play.h"
#include "tasks/webAudio/SetVolume.h"
#include "tasks/webAudio/Shutdown.h"
#include "tasks/webAudio/Stop.h"
#include "tasks/webAudio/SetCaps.h"
#include <gst/gst.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

namespace
{
std::shared_ptr<WebAudioTasksTestsContext> testContext;

constexpr double kVolume{0.7};
} // namespace

WebAudioTasksTestsBase::WebAudioTasksTestsBase()
{
    testContext = std::make_shared<WebAudioTasksTestsContext>();

    gst_init(nullptr, nullptr);

    testContext->m_context.pipeline = &testContext->m_pipeline;
    testContext->m_context.source = &testContext->m_src;
}

WebAudioTasksTestsBase::~WebAudioTasksTestsBase()
{
    testContext.reset();
}

void WebAudioTasksTestsBase::shouldEndOfStreamSuccess()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcEndOfStream(GST_APP_SRC(&testContext->m_src))).WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTasksTestsBase::shouldEndOfStreamFailure()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcEndOfStream(GST_APP_SRC(&testContext->m_src))).WillOnce(Return(GST_FLOW_ERROR));
}

void WebAudioTasksTestsBase::triggerEos()
{
    firebolt::rialto::server::tasks::webaudio::Eos task{testContext->m_context, testContext->m_gstWrapper};
    task.execute();
}

void WebAudioTasksTestsBase::shouldPauseSuccess()
{
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PAUSED)).WillOnce(Return(true));
}

void WebAudioTasksTestsBase::shouldPauseFailure()
{
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PAUSED)).WillOnce(Return(false));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyState(firebolt::rialto::WebAudioPlayerState::FAILURE));
}

void WebAudioTasksTestsBase::triggerPause()
{
    firebolt::rialto::server::tasks::webaudio::Pause task{testContext->m_gstPlayer, &testContext->m_gstPlayerClient};
    task.execute();
}

void WebAudioTasksTestsBase::shouldPlaySuccess()
{
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PLAYING)).WillOnce(Return(true));
}

void WebAudioTasksTestsBase::shouldPlayFailure()
{
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PLAYING)).WillOnce(Return(false));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyState(firebolt::rialto::WebAudioPlayerState::FAILURE));
}

void WebAudioTasksTestsBase::triggerPlay()
{
    firebolt::rialto::server::tasks::webaudio::Play task{testContext->m_gstPlayer, &testContext->m_gstPlayerClient};
    task.execute();
}

void WebAudioTasksTestsBase::shouldGstSetVolume()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstStreamVolumeSetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR, kVolume));
}

void WebAudioTasksTestsBase::triggerSetVolume()
{
    firebolt::rialto::server::tasks::webaudio::SetVolume task{testContext->m_context, testContext->m_gstWrapper, kVolume};
    task.execute();
}

void WebAudioTasksTestsBase::shouldStopWorkerThread()
{
    EXPECT_CALL(testContext->m_gstPlayer, stopWorkerThread());
}

void WebAudioTasksTestsBase::triggerShutdown()
{
    firebolt::rialto::server::tasks::webaudio::Shutdown task{testContext->m_gstPlayer};
    task.execute();
}

void WebAudioTasksTestsBase::shouldChangePlayerStateNull()
{
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_NULL));
}

void WebAudioTasksTestsBase::triggerStop()
{
    firebolt::rialto::server::tasks::webaudio::Stop task{testContext->m_gstPlayer};
    task.execute();
}
