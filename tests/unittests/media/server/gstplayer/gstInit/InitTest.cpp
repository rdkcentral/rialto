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

#include "GstInitialiser.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include "IFactoryAccessor.h"
#include "Matchers.h"
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrEq;
using testing::StrictMock;
using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

class RialtoServerInitGstPlayerTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock{
        std::make_shared<StrictMock<GstWrapperFactoryMock>>()};
    int argc = 2;
    char *argv[3] = {"a", "b", nullptr};

    void SetUp() override { IFactoryAccessor::instance().gstWrapperFactory() = m_gstWrapperFactoryMock; }

    void TearDown() { IFactoryAccessor::instance().gstWrapperFactory() = nullptr; }
};

/**
 * Test the initialisation of gstreamer.
 */
TEST_F(RialtoServerInitGstPlayerTest, Init)
{
    // EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));
    // EXPECT_CALL(*m_gstWrapperMock, gstInit(_, _));
    // EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet()).WillOnce(Return(nullptr));
    // EXPECT_CALL(*m_gstWrapperMock, gstRegistryFindPlugin(nullptr, StrEq("rialtosinks"))).WillOnce(Return(nullptr));
    // bool status = false;
    // EXPECT_NO_THROW(status = gstInitalise(argc, static_cast<char **>(argv)));
    // EXPECT_EQ(status, true);
}
