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

#include <gtest/gtest.h>

#include "ClientControllerMock.h"
#include "Control.h"
#include "ControlClientMock.h"

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

MATCHER_P(controlClientMatcher, cc, "")
{
    const std::weak_ptr<firebolt::rialto::IControlClient> a{arg};
    return a.lock() == cc;
}

class RialtoClientControlTest : public ::testing::Test
{
public:
    RialtoClientControlTest()
        : m_controlClientMock{std::make_shared<StrictMock<ControlClientMock>>()},
          m_clientControllerMock{std::make_shared<StrictMock<ClientControllerMock>>()}
    {
    }

protected:
    std::shared_ptr<StrictMock<ControlClientMock>> m_controlClientMock;
    std::shared_ptr<StrictMock<ClientControllerMock>> m_clientControllerMock;
};

/**
 * Test the factory
 */
TEST_F(RialtoClientControlTest, FactoryFails)
{
    std::shared_ptr<firebolt::rialto::IControlFactory> factory = firebolt::rialto::IControlFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_EQ(factory->createControl(), nullptr);
}

TEST_F(RialtoClientControlTest, CreateDestroy)
{
    std::unique_ptr<IControl> control;

    // Create
    EXPECT_NO_THROW(control = std::make_unique<Control>(*m_clientControllerMock));

    // Destroy
    EXPECT_CALL(*m_clientControllerMock, eventFlush());
    control.reset();
}
