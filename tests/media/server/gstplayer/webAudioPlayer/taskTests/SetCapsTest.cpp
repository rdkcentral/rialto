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

class WebAudioSetCapsTest : public WebAudioTasksTestsBase
{
protected:

    WebAudioSetCapsTest()
    {
        setU32LEConfig();
    }
};

TEST_F(WebAudioSetCapsTest, shouldSetCapsWithFormatF64LE)
{
    setF64LEConfig();

    shouldBuildPcmCaps();
    shouldGetCapsStr();
    shouldSetCaps();
    shouldUnref();
    triggerSetCaps();
    checkBytesPerSamplePcmSet();
}

TEST_F(WebAudioSetCapsTest, shouldSetCapsWithWithFormatS16BE)
{
    setS16BEConfig();

    shouldBuildPcmCaps();
    shouldGetCapsStr();
    shouldSetCaps();
    shouldUnref();
    triggerSetCaps();
    checkBytesPerSamplePcmSet();
}

TEST_F(WebAudioSetCapsTest, shouldSetCapsWithFormatU32LE)
{
    shouldBuildPcmCaps();
    shouldGetCapsStr();
    shouldSetCaps();
    shouldUnref();
    triggerSetCaps();
    checkBytesPerSamplePcmSet();
}

TEST_F(WebAudioSetCapsTest, shouldSetCapsWhenAppSrcCapsNull)
{
    shouldBuildPcmCaps();
    shouldGetCapsStr();
    shouldSetCapsWhenAppSrcCapsNull();
    triggerSetCaps();
}

TEST_F(WebAudioSetCapsTest, shouldNotSetCapsWhenInvalidMimeType)
{
    triggerSetCapsInvalidMimeType();
}

TEST_F(WebAudioSetCapsTest, shouldNotSetCapsWhenCapsEqual)
{
    shouldBuildPcmCaps();
    shouldGetCapsStr();
    shouldUnref();
    shouldNotSetCapsWhenCapsEqual();
    triggerSetCaps();
}
