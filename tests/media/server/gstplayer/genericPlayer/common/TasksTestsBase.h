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

#ifndef TASKS_TESTS_BASE_H_
#define TASKS_TESTS_BASE_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::A;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::StrictMock;

class TasksTestsBase : public ::testing::Test
{
public:
    TasksTestsBase();
    virtual ~TasksTestsBase();

protected:
    void shouldSetupVideoElementOnly();
    void shouldSetupVideoElementWesterossink();
    void shouldSetupVideoElementAmlhalasink();
    void shouldSetupVideoElementPendingGeometryNonWesterissink();
    void shouldSetupAudioElementOnly();
    void shouldSetVideoUnderflowCallback();
    void triggerSetupElement();
    void triggerVideoUnderflowCallback();
    void shouldSetAudioUnderflowCallback();
    void triggerAudioUnderflowCallback();

    void initSetVideoGeometryTest();
    void termSetVideoGeometryTest();
    void setPipelineToNull();
    void triggerSetVideoGeometryFailure();
    void shouldSetVideoGeometry();
    void triggerSetVideoGeometrySuccess();

    void setAllSourcesAttached();
    void shouldScheduleAllSourcesAttached();
    void triggerSetupSource();

    void shouldSetGstVolume();
    void triggerSetVolume();
private:
    void expectSetupVideoElement();
    void expectSetupAudioElement();
};

#endif // TASKS_TESTS_BASE_H_
