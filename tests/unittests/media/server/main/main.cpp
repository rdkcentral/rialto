/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include "IFactoryAccessor.h"
#include "IGstInitialiser.h"
#include <gtest/gtest.h>

void initialiseGstreamer()
{
    using testing::_;
    using testing::Return;
    using testing::StrictMock;
    using namespace firebolt::rialto::wrappers;

    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> gstWrapperFactoryMock{
        std::make_shared<StrictMock<GstWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<GstWrapperMock>> gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
    EXPECT_CALL(*gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(gstWrapperMock));
    IFactoryAccessor::instance().gstWrapperFactory() = gstWrapperFactoryMock;

    EXPECT_CALL(*gstWrapperMock, gstInit(nullptr, nullptr));
    EXPECT_CALL(*gstWrapperMock, gstRegistryGet()).WillOnce(Return(nullptr));
    EXPECT_CALL(*gstWrapperMock, gstRegistryFindPlugin(nullptr, _)).WillOnce(Return(nullptr));
    firebolt::rialto::server::IGstInitialiser::instance().initialise(nullptr, nullptr);
    firebolt::rialto::server::IGstInitialiser::instance().waitForInitialisation();

    IFactoryAccessor::instance().gstWrapperFactory() = nullptr;
}

int main(int argc, char **argv) // NOLINT(build/filename_format)
{
    ::testing::InitGoogleTest(&argc, argv);
    initialiseGstreamer();
    int status = RUN_ALL_TESTS();
    return status;
}
