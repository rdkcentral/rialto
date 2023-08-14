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

#include "GenericTasksTestsBase.h"

class UnderflowTest : public GenericTasksTestsBase
{
protected:
    UnderflowTest()
    {
        setContextAudioAppSrc();
        setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    }
};

TEST_F(UnderflowTest, shouldNotReportUnderflowWhenItIsDisabled)
{
    setUnderflowFlag(false);
    setUnderflowEnabled(false);
    triggerVideoUnderflow();
}

TEST_F(UnderflowTest, shouldNotReportUnderflowWhenItIsAlreadyActive)
{
    setUnderflowFlag(true);
    setUnderflowEnabled(true);
    triggerVideoUnderflow();
    checkUnderflowFlag(true);
}

TEST_F(UnderflowTest, shouldReportUnderflow)
{
    setUnderflowFlag(false);
    setUnderflowEnabled(true);
    shouldNotifyVideoUnderflow();
    triggerVideoUnderflow();
    checkUnderflowFlag(true);
}

TEST_F(UnderflowTest, shouldNotReportEosWhenEosAlreadyNotified)
{
    setContextEndOfStream(firebolt::rialto::MediaSourceType::AUDIO);
    setContextEndOfStreamNotified();
    setUnderflowFlag(false);
    setUnderflowEnabled(true);
    triggerVideoUnderflow();
    checkUnderflowFlag(false);
}

TEST_F(UnderflowTest, shouldReportEos)
{
    setContextEndOfStream(firebolt::rialto::MediaSourceType::AUDIO);
    setUnderflowFlag(false);
    setUnderflowEnabled(true);
    shouldNotifyEndOfStream();
    triggerVideoUnderflow();
    checkUnderflowFlag(false);
    checkEndOfStreamNotified();
}
