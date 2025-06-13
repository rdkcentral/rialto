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

#ifndef WEB_AUDIO_TASKS_TESTS_BASE_H_
#define WEB_AUDIO_TASKS_TESTS_BASE_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

using ::testing::_;
using ::testing::A;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::StrEq;
using ::testing::StrictMock;

/**
 * @brief WebAudioTasksTest Base class
 *
 * This class exists to create a common place for all gstreamer objects and mocks to coexist.
 * Moving all gstreamer dependancies into one file reduces the compile time dramatically.
 */
class WebAudioTasksTestsBase : public ::testing::Test
{
public:
    WebAudioTasksTestsBase();
    virtual ~WebAudioTasksTestsBase();

protected:
    // Eos test methods
    void shouldEndOfStreamSuccess();
    void shouldEndOfStreamFailure();
    void triggerEos();

    // Pause test methods
    void shouldPauseSuccess();
    void shouldPauseFailure();
    void triggerPause();

    // Play test methods
    void shouldPlaySuccess();
    void shouldPlayFailure();
    void triggerPlay();

    // SetVolume test methods
    void shouldGstSetVolume();
    void triggerSetVolume();

    // Stop test methods
    void shouldChangePlayerStateNull();
    void triggerStop();

    // SetCaps test methods
    void setU32LEConfig();
    void setF64LEConfig();
    void setS16BEConfig();
    void shouldBuildPcmCaps();
    void shouldGetCapsStr();
    void shouldSetCaps();
    void shouldUnref();
    void checkBytesPerSamplePcmSet();
    void shouldSetCapsWhenAppSrcCapsNull();
    void shouldNotSetCapsWhenCapsEqual();
    void triggerSetCaps();
    void triggerSetCapsInvalidMimeType();

    // WriteBuffer test methods
    void setContextBytesPerSample();
    void shouldWriteBufferForAllData();
    void triggerWriteBuffer();
    void checkWriteAllData();
    void shouldWriteBufferForAllMainDataAndPartialWrapData();
    void checkWriteAllMainDataAndPartialWrapData();
    void shouldWriteBufferForPartialMainDataAndNoWrapData();
    void checkWritePartialMainDataAndNoWrapData();
    void shouldNotWriteBufferIfNewAllocateFails();
    void checkWriteNoData();
    void shouldWriteBufferIfBytesWrittenLessThanExpected();
    void checkWriteLessThanExpected();
    void shouldNotWriteBufferIfPushBufferFails();
    void shouldNotWriteBufferIfBytesToWriteLessThanBytesPerSample();
    void shouldWriteBufferThatNotAlignedWithBytesPerSample();
    void checkWriteUnaligned();

private:
    // SetCaps helper methods
    std::string getPcmFormat();
};

#endif // WEB_AUDIO_TASKS_TESTS_BASE_H_
