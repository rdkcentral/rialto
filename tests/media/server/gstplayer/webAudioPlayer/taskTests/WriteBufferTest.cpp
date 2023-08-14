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

class WebAudioWriteBufferTest : public WebAudioTasksTestsBase
{
protected:
    WebAudioWriteBufferTest() { setContextBytesPerSample(); }
};

TEST_F(WebAudioWriteBufferTest, shouldWriteBufferForAllData)
{
    shouldWriteBufferForAllData();
    triggerWriteBuffer();
    checkWriteAllData();
}

TEST_F(WebAudioWriteBufferTest, shouldWriteBufferForAllMainDataAndPartialWrapData)
{
    shouldWriteBufferForAllMainDataAndPartialWrapData();
    triggerWriteBuffer();
    checkWriteAllMainDataAndPartialWrapData();
}

TEST_F(WebAudioWriteBufferTest, shouldWriteBufferForPartialMainDataAndNoWrapData)
{
    shouldWriteBufferForPartialMainDataAndNoWrapData();
    triggerWriteBuffer();
    checkWritePartialMainDataAndNoWrapData();
}

TEST_F(WebAudioWriteBufferTest, shouldNotWriteBufferIfNewAllocateFails)
{
    shouldNotWriteBufferIfNewAllocateFails();
    triggerWriteBuffer();
    checkWriteNoData();
}

TEST_F(WebAudioWriteBufferTest, shouldWriteBufferIfBytesWrittenLessThanExpected)
{
    shouldWriteBufferIfBytesWrittenLessThanExpected();
    triggerWriteBuffer();
    checkWriteLessThanExpected();
}

TEST_F(WebAudioWriteBufferTest, shouldNotWriteBufferIfPushBufferFails)
{
    shouldNotWriteBufferIfPushBufferFails();
    triggerWriteBuffer();
    checkWriteNoData();
}

TEST_F(WebAudioWriteBufferTest, shouldNotWriteBufferIfBytesToWriteLessThanBytesPerSample)
{
    shouldNotWriteBufferIfBytesToWriteLessThanBytesPerSample();
    triggerWriteBuffer();
    checkWriteNoData();
}

TEST_F(WebAudioWriteBufferTest, shouldWriteBufferThatNotAlignedWithBytesPerSample)
{
    shouldWriteBufferThatNotAlignedWithBytesPerSample();
    triggerWriteBuffer();
    checkWriteUnaligned();
}
