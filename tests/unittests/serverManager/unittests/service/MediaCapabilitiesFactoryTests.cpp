/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

// ASSUMPTION (verify against your actual headers): IYamlCppWrapperFactory::getFactory() is a
// static method, and this codebase provides a test-only injection seam - shown here as a
// "FactoryAccessor" singleton, matching the pattern used elsewhere in Rialto for similar
// static wrapper factories (e.g. IGstWrapperFactory::getFactory(), IGlibWrapperFactory::getFactory()).
// If the real injection mechanism differs (different singleton name, a setFactory()/resetFactory()
// pair, etc.), only the fixture's constructor/destructor below need to change - the three TEST_F
// bodies are independent of how the mock gets wired in.
#include "FactoryAccessor.h"
#include "MediaCapabilitiesFactory.h"
#include "YamlCppWrapperMock.h"
#include <gtest/gtest.h>

using namespace rialto::servermanager::service;
using namespace firebolt::rialto::wrappers;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

class MediaCapabilitiesFactoryTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<YamlCppWrapperFactoryMock>> m_yamlCppWrapperFactoryMock{
        std::make_shared<StrictMock<YamlCppWrapperFactoryMock>>()};

    MediaCapabilitiesFactoryTest()
    {
        // The static singleton accessor lives on the INTERFACE (IFactoryAccessor::instance()),
        // matching YamlCppWrapperAccessor.cpp's own call:
        // IFactoryAccessor::instance().yamlCppWrapperFactory().
        IFactoryAccessor::instance().yamlCppWrapperFactory() = m_yamlCppWrapperFactoryMock;
    }

    ~MediaCapabilitiesFactoryTest() override
    {
        // Reset the injection point so later tests in the same binary aren't affected.
        IFactoryAccessor::instance().yamlCppWrapperFactory() = nullptr;
    }
};

TEST_F(MediaCapabilitiesFactoryTest, ReturnsNullptrWhenFactoryUnavailable)
{
    IFactoryAccessor::instance().yamlCppWrapperFactory() = nullptr;

    auto result = createMediaCapabilities();

    EXPECT_EQ(result, nullptr);
}

TEST_F(MediaCapabilitiesFactoryTest, ReturnsNullptrWhenYamlCppWrapperCreationFails)
{
    // createYamlCppWrapper() returns std::shared_ptr<IYamlCppWrapper> - copyable, so Return(...)
    // is used directly; ByMove(...) is only needed for move-only return types (e.g. unique_ptr).
    EXPECT_CALL(*m_yamlCppWrapperFactoryMock, createYamlCppWrapper()).WillOnce(Return(nullptr));

    auto result = createMediaCapabilities();

    EXPECT_EQ(result, nullptr);
}

TEST_F(MediaCapabilitiesFactoryTest, CreatesMediaCapabilitiesWhenDependenciesAvailable)
{
    // Must be shared_ptr (matching createYamlCppWrapper()'s real return type) AND the SAME
    // object must be returned from the mock - returning a fresh/empty shared_ptr here would
    // silently make this test degenerate into the "creation fails" case above.
    auto yamlCppWrapper = std::make_shared<StrictMock<YamlCppWrapperMock>>();
    EXPECT_CALL(*m_yamlCppWrapperFactoryMock, createYamlCppWrapper()).WillOnce(Return(yamlCppWrapper));

    auto result = createMediaCapabilities();

    ASSERT_NE(result, nullptr);
}
