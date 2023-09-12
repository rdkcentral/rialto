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

namespace
{
constexpr int kControlId{8};
} // namespace

TEST_F(ControlServiceTests, shouldNotAckWithoutAddedControl)
{
    EXPECT_FALSE(triggerAck(kControlId));
}

TEST_F(ControlServiceTests, shouldNotSetApplicationStateWithoutAddedControl)
{
    triggerSetApplicationState();
}

TEST_F(ControlServiceTests, shouldNotAckWithUnknownControlId)
{
    controlServerInternalFactoryWillCreateControlServerInternal(kControlId);
    triggerAddControl(kControlId);
    EXPECT_FALSE(triggerAck(kControlId + 1));
}

TEST_F(ControlServiceTests, shouldNotAckWithRemovedControlId)
{
    controlServerInternalFactoryWillCreateControlServerInternal(kControlId);
    triggerAddControl(kControlId);
    triggerRemoveControl(kControlId);
    triggerSetApplicationState();
}

TEST_F(ControlServiceTests, shouldNotSetApplicationStateWithRemovedControlId)
{
    controlServerInternalFactoryWillCreateControlServerInternal(kControlId);
    triggerAddControl(kControlId);
    triggerRemoveControl(kControlId);
    EXPECT_FALSE(triggerAck(kControlId));
}

TEST_F(ControlServiceTests, shouldAck)
{
    controlServerInternalFactoryWillCreateControlServerInternal(kControlId);
    controlServerInternalWillAck();
    triggerAddControl(kControlId);
    EXPECT_TRUE(triggerAck(kControlId));
}

TEST_F(ControlServiceTests, shouldSetApplicationState)
{
    controlServerInternalFactoryWillCreateControlServerInternal(kControlId);
    controlServerInternalWillSetApplicationState();
    triggerAddControl(kControlId);
    triggerSetApplicationState();
}

TEST_F(ControlServiceTests, shouldSetApplicationStateForNewControl)
{
    triggerSetApplicationState();
    controlServerInternalFactoryWillCreateControlServerInternalWithSetState(kControlId);
    triggerAddControl(kControlId);
}

TEST_F(ControlServiceTests, shouldPing)
{
    controlServerInternalFactoryWillCreateControlServerInternal(kControlId);
    controlServerInternalWillPing();
    triggerAddControl(kControlId);
    EXPECT_TRUE(triggerPing());
}
