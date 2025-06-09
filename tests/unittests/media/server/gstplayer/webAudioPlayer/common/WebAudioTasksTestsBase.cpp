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

#include <gst/gst.h>
#include <memory>
#include <string>

#include "Matchers.h"
#include "WebAudioTasksTestsBase.h"
#include "WebAudioTasksTestsContext.h"
#include "WebAudioTestCommon.h"
#include "tasks/webAudio/Eos.h"
#include "tasks/webAudio/Pause.h"
#include "tasks/webAudio/Play.h"
#include "tasks/webAudio/SetCaps.h"
#include "tasks/webAudio/SetVolume.h"
#include "tasks/webAudio/Shutdown.h"
#include "tasks/webAudio/Stop.h"
#include "tasks/webAudio/WriteBuffer.h"

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

namespace
{
std::shared_ptr<WebAudioTasksTestsContext> testContext;

constexpr double kVolume{0.7};
constexpr uint64_t kChannelMask{5};
constexpr uint32_t kBytesPerSample{4};
constexpr uint32_t kMainLength = firebolt::rialto::server::kMaxWebAudioBytes - 40;
constexpr uint32_t kWrapLength = 40;
constexpr uint32_t kUnalignedWriteSize = kMainLength + kWrapLength - kBytesPerSample;
const std::string kAudioMimeType{"audio/x-raw"};
} // namespace

WebAudioTasksTestsBase::WebAudioTasksTestsBase()
{
    testContext = std::make_shared<WebAudioTasksTestsContext>();

    testContext->m_context.pipeline = &testContext->m_pipeline;
    testContext->m_context.source = &testContext->m_src;
}

WebAudioTasksTestsBase::~WebAudioTasksTestsBase()
{
    testContext.reset();
}

void WebAudioTasksTestsBase::shouldEndOfStreamSuccess()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcEndOfStream(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTasksTestsBase::shouldEndOfStreamFailure()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcEndOfStream(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(GST_FLOW_ERROR));
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
    // The real (non-mock) changePipelineState will notify the client of failure
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
    // The real (non-mock) changePipelineState will notify the client of failure
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

void WebAudioTasksTestsBase::setU32LEConfig()
{
    testContext->m_config->pcm.rate = 1;
    testContext->m_config->pcm.channels = 2;
    testContext->m_config->pcm.sampleSize = 32;
    testContext->m_config->pcm.isBigEndian = false;
    testContext->m_config->pcm.isSigned = false;
    testContext->m_config->pcm.isFloat = false;
}

void WebAudioTasksTestsBase::setF64LEConfig()
{
    testContext->m_config->pcm.rate = 1;
    testContext->m_config->pcm.channels = 2;
    testContext->m_config->pcm.sampleSize = 64;
    testContext->m_config->pcm.isBigEndian = false;
    testContext->m_config->pcm.isSigned = false;
    testContext->m_config->pcm.isFloat = true;
}

void WebAudioTasksTestsBase::setS16BEConfig()
{
    testContext->m_config->pcm.rate = 1;
    testContext->m_config->pcm.channels = 2;
    testContext->m_config->pcm.sampleSize = 16;
    testContext->m_config->pcm.isSigned = true;
    testContext->m_config->pcm.isBigEndian = true;
    testContext->m_config->pcm.isFloat = false;
}

std::string WebAudioTasksTestsBase::getPcmFormat()
{
    return testcommon::getPcmFormat(testContext->m_config->pcm.isFloat, testContext->m_config->pcm.isSigned,
                                    testContext->m_config->pcm.sampleSize, testContext->m_config->pcm.isBigEndian);
}

void WebAudioTasksTestsBase::shouldBuildPcmCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/x-raw")))
        .WillOnce(Return(&testContext->m_caps));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleIntStub(&testContext->m_caps, StrEq("channels"), G_TYPE_INT,
                                                                    testContext->m_config->pcm.channels));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_caps, StrEq("layout"), G_TYPE_STRING, StrEq("interleaved")));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleIntStub(&testContext->m_caps, StrEq("rate"), G_TYPE_INT,
                                                                    testContext->m_config->pcm.rate));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleStringStub(&testContext->m_caps, StrEq("format"),
                                                                       G_TYPE_STRING, StrEq(getPcmFormat().c_str())));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAudioChannelGetFallbackMask(testContext->m_config->pcm.channels))
        .WillOnce(Return(kChannelMask));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleBitMaskStub(&testContext->m_caps, StrEq("channel-mask"),
                                                                        GST_TYPE_BITMASK, kChannelMask));
}

void WebAudioTasksTestsBase::shouldGetCapsStr()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_caps)).WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
}

void WebAudioTasksTestsBase::shouldSetCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(&testContext->m_capsAppSrc));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsIsEqual(&testContext->m_capsAppSrc, &testContext->m_caps))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_src), &testContext->m_caps));
}

void WebAudioTasksTestsBase::shouldUnref()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_capsAppSrc));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_caps));
}

void WebAudioTasksTestsBase::checkBytesPerSamplePcmSet()
{
    uint32_t expectedBytesPerSample = testContext->m_config->pcm.channels *
                                      (testContext->m_config->pcm.sampleSize / CHAR_BIT);
    EXPECT_EQ(expectedBytesPerSample, testContext->m_context.bytesPerSample);
}

void WebAudioTasksTestsBase::shouldSetCapsWhenAppSrcCapsNull()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&testContext->m_src))).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_src), &testContext->m_caps));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_caps));
}

void WebAudioTasksTestsBase::shouldNotSetCapsWhenCapsEqual()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(&testContext->m_capsAppSrc));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsIsEqual(&testContext->m_capsAppSrc, &testContext->m_caps))
        .WillOnce(Return(TRUE));
}

void WebAudioTasksTestsBase::triggerSetCaps()
{
    firebolt::rialto::server::tasks::webaudio::SetCaps task{testContext->m_context, testContext->m_gstWrapper,
                                                            testContext->m_glibWrapper, kAudioMimeType,
                                                            testContext->m_config};
    task.execute();
}

void WebAudioTasksTestsBase::triggerSetCapsInvalidMimeType()
{
    firebolt::rialto::server::tasks::webaudio::SetCaps task{testContext->m_context, testContext->m_gstWrapper,
                                                            testContext->m_glibWrapper, "invalid", testContext->m_config};
    task.execute();
}

void WebAudioTasksTestsBase::setContextBytesPerSample()
{
    testContext->m_context.bytesPerSample = kBytesPerSample;
}

void WebAudioTasksTestsBase::shouldWriteBufferForAllData()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(0));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewAllocate(_, kMainLength + kWrapLength, _))
        .WillOnce(Return(&testContext->m_buffer));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, 0, &testContext->m_mainPtr, kMainLength))
        .WillOnce(Return(kMainLength));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, kMainLength, &testContext->m_wrapPtr, kWrapLength))
        .WillOnce(Return(kWrapLength));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&testContext->m_src), &testContext->m_buffer))
        .WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTasksTestsBase::triggerWriteBuffer()
{
    firebolt::rialto::server::tasks::webaudio::WriteBuffer task{testContext->m_context,  testContext->m_gstWrapper,
                                                                &testContext->m_mainPtr, kMainLength,
                                                                &testContext->m_wrapPtr, kWrapLength};
    task.execute();
}

void WebAudioTasksTestsBase::checkWriteAllData()
{
    EXPECT_EQ(testContext->m_context.lastBytesWritten, kMainLength + kWrapLength);
}

void WebAudioTasksTestsBase::shouldWriteBufferForAllMainDataAndPartialWrapData()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(kWrapLength / 2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewAllocate(_, kMainLength + kWrapLength / 2, _))
        .WillOnce(Return(&testContext->m_buffer));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, 0, &testContext->m_mainPtr, kMainLength))
        .WillOnce(Return(kMainLength));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, kMainLength, &testContext->m_wrapPtr, kWrapLength / 2))
        .WillOnce(Return(kWrapLength / 2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&testContext->m_src), &testContext->m_buffer))
        .WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTasksTestsBase::checkWriteAllMainDataAndPartialWrapData()
{
    EXPECT_EQ(testContext->m_context.lastBytesWritten, kMainLength + kWrapLength / 2);
}

void WebAudioTasksTestsBase::shouldWriteBufferForPartialMainDataAndNoWrapData()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(kMainLength / 2 + kWrapLength));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewAllocate(_, kMainLength / 2, _))
        .WillOnce(Return(&testContext->m_buffer));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, 0, &testContext->m_mainPtr, kMainLength / 2))
        .WillOnce(Return(kMainLength / 2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&testContext->m_src), &testContext->m_buffer))
        .WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTasksTestsBase::checkWritePartialMainDataAndNoWrapData()
{
    EXPECT_EQ(testContext->m_context.lastBytesWritten, kMainLength / 2);
}

void WebAudioTasksTestsBase::shouldNotWriteBufferIfNewAllocateFails()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(0));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewAllocate(_, kMainLength + kWrapLength, _)).WillOnce(Return(nullptr));
}

void WebAudioTasksTestsBase::checkWriteNoData()
{
    EXPECT_EQ(testContext->m_context.lastBytesWritten, 0);
}

void WebAudioTasksTestsBase::shouldWriteBufferIfBytesWrittenLessThanExpected()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(0));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewAllocate(_, kMainLength + kWrapLength, _))
        .WillOnce(Return(&testContext->m_buffer));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, 0, &testContext->m_mainPtr, kMainLength))
        .WillOnce(Return(kMainLength - 1));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, kMainLength - 1, &testContext->m_wrapPtr, kWrapLength))
        .WillOnce(Return(kWrapLength));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&testContext->m_src), &testContext->m_buffer))
        .WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTasksTestsBase::checkWriteLessThanExpected()
{
    EXPECT_EQ(testContext->m_context.lastBytesWritten, kMainLength + kWrapLength - 1);
}

void WebAudioTasksTestsBase::shouldNotWriteBufferIfPushBufferFails()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(0));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewAllocate(_, kMainLength + kWrapLength, _))
        .WillOnce(Return(&testContext->m_buffer));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, 0, &testContext->m_mainPtr, kMainLength))
        .WillOnce(Return(kMainLength));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, kMainLength, &testContext->m_wrapPtr, kWrapLength))
        .WillOnce(Return(kWrapLength));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&testContext->m_src), &testContext->m_buffer))
        .WillOnce(Return(GST_FLOW_ERROR));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&testContext->m_buffer));
}

void WebAudioTasksTestsBase::shouldNotWriteBufferIfBytesToWriteLessThanBytesPerSample()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(firebolt::rialto::server::kMaxWebAudioBytes - testContext->m_context.bytesPerSample + 1));
}

void WebAudioTasksTestsBase::shouldWriteBufferThatNotAlignedWithBytesPerSample()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&testContext->m_src)))
        .WillOnce(Return(testContext->m_context.bytesPerSample - 1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewAllocate(_, kUnalignedWriteSize, _))
        .WillOnce(Return(&testContext->m_buffer));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstBufferFill(&testContext->m_buffer, 0, &testContext->m_mainPtr, kMainLength))
        .WillOnce(Return(kMainLength));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferFill(&testContext->m_buffer, kMainLength, &testContext->m_wrapPtr,
                                                          kUnalignedWriteSize - kMainLength))
        .WillOnce(Return(kUnalignedWriteSize - kMainLength));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&testContext->m_src), &testContext->m_buffer))
        .WillOnce(Return(GST_FLOW_OK));
}

void WebAudioTasksTestsBase::checkWriteUnaligned()
{
    EXPECT_EQ(testContext->m_context.lastBytesWritten, kUnalignedWriteSize);
}
