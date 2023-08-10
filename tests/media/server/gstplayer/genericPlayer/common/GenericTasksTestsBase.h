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

#ifndef GENERIC_TASKS_TESTS_BASE_H_
#define GENERIC_TASKS_TESTS_BASE_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

/**
 * @brief GenericTasksTest Base class
 *
 * This class exists to create a common place for all gstreamer objects and mocks to coexist.
 * Moving all gstreamer dependancies into one file reduces the compile time dramatically.
 */
class GenericTasksTestsBase : public ::testing::Test
{
public:
    GenericTasksTestsBase();
    virtual ~GenericTasksTestsBase();

protected:
    // SetupElement test methods
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

    // SetVideoGeometry test methods
    void setPipelineToNull();
    void triggerSetVideoGeometryFailure();
    void shouldSetVideoGeometry();
    void triggerSetVideoGeometrySuccess();

    // SetupSource test methods
    void setAllSourcesAttached();
    void shouldScheduleAllSourcesAttached();
    void triggerSetupSource();

    // SetVolume test methods
    void shouldSetGstVolume();
    void triggerSetVolume();

private:
    // SetupElement helper methods
    void expectSetupVideoElement();
    void expectSetupAudioElement();
};

#endif // GENERIC_TASKS_TESTS_BASE_H_
