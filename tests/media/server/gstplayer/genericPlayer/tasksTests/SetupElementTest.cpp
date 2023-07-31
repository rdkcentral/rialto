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

class SetupElementTest : public TasksTestsBase
{
};

TEST_F(SetupElementTest, shouldSetupVideoElement)
{
    shouldSetupVideoElementOnly();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithPendingGeometry)
{
    shouldSetupVideoElementWesterossink();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementForAmlhalasink)
{
    shouldSetupVideoElementAmlhalasink();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithPendingGeometryOtherThanWesterosSink)
{
    shouldSetupVideoElementPendingGeometryNonWesterissink();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElement)
{
    shouldSetupAudioElementOnly();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldReportVideoUnderflow)
{
    shouldSetupVideoElementOnly();
    triggerSetupElement();

    shouldSetVideoUnderflowCallback();
    triggerVideoUnderflowCallback();
}

TEST_F(SetupElementTest, shouldReportAudioUnderflow)
{
    shouldSetupAudioElementOnly();
    triggerSetupElement();

    shouldSetAudioUnderflowCallback();
    triggerAudioUnderflowCallback();
}
