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

#include "ControlServiceTestsFixture.h"

TEST_F(ControlServiceTests, shouldNotAckWithoutAddedControl)
{
    constexpr int kControlId{8};
    EXPECT_FALSE(triggerAck(kControlId));
}

TEST_F(ControlServiceTests, shouldNotSetApplicationStateWithoutAddedControl)
{
    triggerSetApplicationState();
}

TEST_F(ControlServiceTests, shouldNotAckWithUnknownControlId)
{
    controlServerInternalFactoryWillCreateControlServerInternal();
    int validControlId{triggerAddControl()};
    EXPECT_FALSE(triggerAck(validControlId + 1));
}

TEST_F(ControlServiceTests, shouldNotAckWithRemovedControlId)
{
    controlServerInternalFactoryWillCreateControlServerInternal();
    int validControlId{triggerAddControl()};
    triggerRemoveControl(validControlId);
    triggerSetApplicationState();
}

TEST_F(ControlServiceTests, shouldNotSetApplicationStateWithRemovedControlId)
{
    controlServerInternalFactoryWillCreateControlServerInternal();
    int validControlId{triggerAddControl()};
    triggerRemoveControl(validControlId);
    EXPECT_FALSE(triggerAck(validControlId));
}

TEST_F(ControlServiceTests, shouldAck)
{
    controlServerInternalFactoryWillCreateControlServerInternal();
    controlServerInternalWillAck();
    int validControlId{triggerAddControl()};
    EXPECT_TRUE(triggerAck(validControlId));
}

TEST_F(ControlServiceTests, shouldSetApplicationState)
{
    controlServerInternalFactoryWillCreateControlServerInternal();
    controlServerInternalWillSetApplicationState();
    triggerAddControl();
    triggerSetApplicationState();
}

TEST_F(ControlServiceTests, shouldSetApplicationStateForNewControl)
{
    triggerSetApplicationState();
    controlServerInternalFactoryWillCreateControlServerInternalWithSetState();
    triggerAddControl();
}
