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

#include "GstInitialiser.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include "IFactoryAccessor.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

void initialiseGstreamer()
{
    using testing::_;
    using testing::Return;
    using testing::StrictMock;
    using namespace firebolt::rialto::wrappers;

    gst_init(nullptr, nullptr);

    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> gstWrapperFactoryMock{
        std::make_shared<StrictMock<GstWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<GstWrapperMock>> gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
    EXPECT_CALL(*gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(gstWrapperMock));
    IFactoryAccessor::instance().gstWrapperFactory() = gstWrapperFactoryMock;

    // Hack to mock GstPlugin to cover one of ifs in code
    uint8_t dummyMemory;
    GstPlugin *plugin{reinterpret_cast<GstPlugin *>(&dummyMemory)};
    EXPECT_CALL(*gstWrapperMock, gstInit(nullptr, nullptr));
    EXPECT_CALL(*gstWrapperMock, gstRegistryGet()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*gstWrapperMock, gstRegistryFindPlugin(nullptr, _)).WillOnce(Return(plugin));
    EXPECT_CALL(*gstWrapperMock, gstRegistryRemovePlugin(nullptr, plugin));
    EXPECT_CALL(*gstWrapperMock, gstObjectUnref(plugin));
    firebolt::rialto::server::IGstInitialiser::instance().initialise(nullptr, nullptr);
    firebolt::rialto::server::IGstInitialiser::instance().waitForInitialisation();

    IFactoryAccessor::instance().gstWrapperFactory() = nullptr;
}

int main(int argc, char **argv) // NOLINT(build/filename_format)
{
    ::testing::InitGoogleTest(&argc, argv);
    initialiseGstreamer();
    int status = RUN_ALL_TESTS();
    gst_deinit();
    return status;
}
