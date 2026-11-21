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

#include "CaptureHalLibrary.h"
#include "FakeCaptureHalControl.h"
#include "ICaptureSession.h"
#include "RialtoLogging.h"

#include <dlfcn.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>

using namespace firebolt::rialto;
using namespace firebolt::rialto::capture;
using namespace firebolt::rialto::wrappers;

namespace
{
const std::string kFakeHal{FAKE_CAPTURE_HAL_PATH};
const std::string kFakeHalWithoutFactory{FAKE_CAPTURE_HAL_WITHOUT_FACTORY_PATH};

// True if a library whose path ends with \a fileName is mapped into this process.
bool isMapped(const std::string &path)
{
    const std::string kFileName = path.substr(path.rfind('/') + 1);
    std::ifstream maps{"/proc/self/maps"};
    std::string line;
    while (std::getline(maps, line))
    {
        if (line.size() >= kFileName.size() &&
            line.compare(line.size() - kFileName.size(), kFileName.size(), kFileName) == 0)
            return true;
    }
    return false;
}

// Calls a control function of the already-loaded fake library without keeping it loaded.
template <typename Function, typename... Args> auto callFake(const char *name, Args... args)
{
    void *handle = dlopen(kFakeHal.c_str(), RTLD_NOW | RTLD_NOLOAD);
    EXPECT_NE(handle, nullptr) << "fake capture HAL is not loaded";
    auto function = reinterpret_cast<Function>(dlsym(handle, name));
    EXPECT_NE(function, nullptr) << name;
    auto result = function(args...);
    dlclose(handle);
    return result;
}

void callFakeVoid(const char *name, bool value)
{
    void *handle = dlopen(kFakeHal.c_str(), RTLD_NOW | RTLD_NOLOAD);
    ASSERT_NE(handle, nullptr);
    auto function = reinterpret_cast<FakeCaptureHalSetFactoryReturnsNullFunction>(dlsym(handle, name));
    ASSERT_NE(function, nullptr);
    function(value);
    dlclose(handle);
}

// Counts server log lines that contain a text, at any level.
class ServerLog
{
public:
    explicit ServerLog(std::string text) : m_text{std::move(text)}
    {
        logging::setLogHandler(
            RIALTO_COMPONENT_SERVER,
            [this](RIALTO_DEBUG_LEVEL, const char *, int, const char *, const char *message, std::size_t length)
            {
                if (std::string(message, length).find(m_text) != std::string::npos)
                    ++m_count;
            },
            true);
    }
    ~ServerLog() { logging::setLogHandler(RIALTO_COMPONENT_SERVER, nullptr, false); }
    int count() const { return m_count; }

private:
    std::string m_text;
    int m_count{0};
};
} // namespace

class CaptureHalLibraryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        unsetenv(CaptureHalLibrary::kLibraryNameEnv);
        ASSERT_FALSE(isMapped(kFakeHal)) << "a previous test left the fake capture HAL loaded";
    }

    void TearDown() override { unsetenv(CaptureHalLibrary::kLibraryNameEnv); }

    void useLibrary(const std::string &path) { setenv(CaptureHalLibrary::kLibraryNameEnv, path.c_str(), 1); }
};

TEST_F(CaptureHalLibraryTest, ShouldLoadOverriddenLibraryAndCreateSession)
{
    useLibrary(kFakeHal);
    CaptureHalLibrary library;

    EXPECT_TRUE(library.isAvailable());
    CaptureSessionPtr session = library.createSession();
    ASSERT_NE(session, nullptr);

    CaptureCapabilities capabilities;
    EXPECT_EQ(session->getCapabilities(capabilities), CaptureStatus::OK);
    EXPECT_EQ(capabilities.drmFormat, 0x3231564eU);
    EXPECT_EQ(callFake<FakeCaptureHalCountFunction>("fakeCaptureHalLiveSessions"), 1);
}

TEST_F(CaptureHalLibraryTest, ShouldUseDefaultLibraryNameWithoutOverride)
{
    ServerLog log{CaptureHalLibrary::kDefaultLibraryName};
    CaptureHalLibrary library;

    // lib_capture_hal.so is not installed on the build machine
    EXPECT_FALSE(library.isAvailable());
    EXPECT_EQ(log.count(), 1);
}

TEST_F(CaptureHalLibraryTest, ShouldReportMissingLibraryAsUnavailableAndLogOnce)
{
    useLibrary("/nonexistent/lib_capture_hal.so");
    ServerLog log{"cannot load"};
    CaptureHalLibrary library;

    EXPECT_FALSE(library.isAvailable());
    EXPECT_FALSE(library.isAvailable());
    EXPECT_EQ(library.createSession(), nullptr);
    EXPECT_EQ(log.count(), 1);
}

TEST_F(CaptureHalLibraryTest, ShouldReportMissingFactorySymbolAsUnavailableAndLogOnce)
{
    useLibrary(kFakeHalWithoutFactory);
    ServerLog log{"has no createCaptureSession"};
    CaptureHalLibrary library;

    EXPECT_FALSE(library.isAvailable());
    EXPECT_FALSE(library.isAvailable());
    EXPECT_EQ(library.createSession(), nullptr);
    EXPECT_EQ(log.count(), 1);
    EXPECT_FALSE(isMapped(kFakeHalWithoutFactory)); // closed again after the failed lookup
}

TEST_F(CaptureHalLibraryTest, ShouldFailWithoutCrashingWhenFactoryReturnsNull)
{
    useLibrary(kFakeHal);
    CaptureHalLibrary library;
    ASSERT_TRUE(library.isAvailable());
    callFakeVoid("fakeCaptureHalSetFactoryReturnsNull", true);

    EXPECT_EQ(library.createSession(), nullptr);
    EXPECT_TRUE(library.isAvailable()); // still available for later attempts

    callFakeVoid("fakeCaptureHalSetFactoryReturnsNull", false);
    EXPECT_NE(library.createSession(), nullptr);
}

TEST_F(CaptureHalLibraryTest, ShouldKeepLibraryLoadedUntilLastSessionIsDeleted)
{
    useLibrary(kFakeHal);
    auto library = std::make_shared<CaptureHalLibrary>();
    CaptureSessionPtr session = library->createSession();
    ASSERT_NE(session, nullptr);
    ASSERT_TRUE(isMapped(kFakeHal));

    library.reset(); // the loader goes first
    EXPECT_TRUE(isMapped(kFakeHal));
    EXPECT_EQ(callFake<FakeCaptureHalCountFunction>("fakeCaptureHalLiveSessions"), 1);

    session.reset(); // the vendor destructor runs, then the library may unload
    EXPECT_FALSE(isMapped(kFakeHal));
}

TEST_F(CaptureHalLibraryTest, ShouldUnloadAfterLoaderWhenNoSessionExists)
{
    useLibrary(kFakeHal);
    {
        CaptureHalLibrary library;
        ASSERT_TRUE(library.isAvailable());
        EXPECT_TRUE(isMapped(kFakeHal));
    }
    EXPECT_FALSE(isMapped(kFakeHal));
}

TEST_F(CaptureHalLibraryTest, FactoryShouldShareLibraryWhileItIsHeld)
{
    CaptureHalLibraryFactory factory;
    auto first = factory.getCaptureHalLibrary();
    auto second = factory.getCaptureHalLibrary();
    EXPECT_EQ(first, second);

    first.reset();
    second.reset();
    EXPECT_NE(factory.getCaptureHalLibrary(), nullptr); // a new one once all are gone
}
