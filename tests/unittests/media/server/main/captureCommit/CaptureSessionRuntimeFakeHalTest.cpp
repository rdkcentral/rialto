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

#include "CaptureSessionRuntime.h"
#include "FakeCaptureHalControl.h"

#include <gtest/gtest.h>

#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>

#include <string>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

namespace
{
// Open descriptors, in this process, of frames the fake HAL exported.
int openFrameFiles()
{
    int count{0};
    DIR *dir = opendir("/proc/self/fd");
    if (!dir)
        return -1;
    while (dirent *entry = readdir(dir))
    {
        char link[256]{};
        const std::string kPath{std::string{"/proc/self/fd/"} + entry->d_name};
        const ssize_t kLength = readlink(kPath.c_str(), link, sizeof(link) - 1);
        if (kLength > 0 && std::string{link, static_cast<std::size_t>(kLength)}.rfind("/memfd:fake-capture-frame", 0) == 0)
            ++count;
    }
    closedir(dir);
    return count;
}
} // namespace

/**
 * The session runtime over the fake capture HAL library, which exports real descriptors (R9 2.3).
 */
class CaptureSessionRuntimeFakeHalTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_hal = dlopen(FAKE_CAPTURE_HAL_PATH, RTLD_NOW | RTLD_LOCAL);
        ASSERT_NE(m_hal, nullptr);
        auto create = reinterpret_cast<capture::ICaptureSession *(*)()>(dlsym(m_hal, "createCaptureSession"));
        m_reset = reinterpret_cast<FakeCaptureHalResetFunction>(dlsym(m_hal, "fakeCaptureHalReset"));
        m_setInvalid = reinterpret_cast<FakeCaptureHalSetNextFrameInvalidFunction>(
            dlsym(m_hal, "fakeCaptureHalSetNextFrameInvalid"));
        m_callCount = reinterpret_cast<FakeCaptureHalCallCountFunction>(dlsym(m_hal, "fakeCaptureHalCallCount"));
        ASSERT_TRUE(create && m_reset && m_setInvalid && m_callCount);
        m_reset();

        capture::ICaptureSession *session{create()};
        ASSERT_NE(session, nullptr);
        capture::CapturePool pool;
        ASSERT_EQ(session->createPool(3, pool), capture::CaptureStatus::OK); // what admission (R8) will do
        m_runtime =
            std::make_unique<CaptureSessionRuntime>(wrappers::CaptureSessionPtr{session,
                                                                                [](capture::ICaptureSession *deleted)
                                                                                { delete deleted; }},
                                                    1, pool);
    }

    void TearDown() override
    {
        m_runtime.reset();
        for (int fd : m_replyFds)
            close(fd);
        if (m_hal)
            dlclose(m_hal);
    }

    // Acquires; the reply's descriptors are kept as the IPC would until it sends them.
    capture::CaptureStatus acquire(CaptureSessionRuntime::DeliveryTicket &ticket)
    {
        return m_runtime->acquire(
            [this](const capture::CapturedFrame &frame, bool, bool)
            {
                for (const auto &kExported : frame.exportedDmaBufs)
                    m_replyFds.push_back(kExported.fd);
            },
            ticket);
    }

    void *m_hal{nullptr};
    FakeCaptureHalResetFunction m_reset{nullptr};
    FakeCaptureHalSetNextFrameInvalidFunction m_setInvalid{nullptr};
    FakeCaptureHalCallCountFunction m_callCount{nullptr};
    std::unique_ptr<CaptureSessionRuntime> m_runtime;
    std::vector<int> m_replyFds;
};

TEST_F(CaptureSessionRuntimeFakeHalTest, InvalidFrameIsReleasedClosedAndMakesTheSessionFatal)
{
    // an earlier, valid frame the client holds
    CaptureSessionRuntime::DeliveryTicket earlier;
    ASSERT_EQ(acquire(earlier), capture::CaptureStatus::OK);
    m_runtime->onDelivery(earlier, true);
    EXPECT_EQ(openFrameFiles(), 1);

    // an invalid frame: released at once, its descriptor closed, the session fatal
    m_setInvalid(true);
    CaptureSessionRuntime::DeliveryTicket invalid;
    EXPECT_EQ(acquire(invalid), capture::CaptureStatus::FATAL_ERROR);
    EXPECT_EQ(m_callCount(FAKE_CAPTURE_HAL_RELEASE_FRAME), 1);
    EXPECT_EQ(openFrameFiles(), 1); // only the earlier frame's
    EXPECT_TRUE(m_runtime->isFatal());
    EXPECT_EQ(m_replyFds.size(), 1U); // nothing of the invalid frame reached a reply

    // later acquisitions fail without asking the HAL
    const int kAcquiresSoFar{m_callCount(FAKE_CAPTURE_HAL_ACQUIRE_CURRENT_FRAME)};
    CaptureSessionRuntime::DeliveryTicket later;
    EXPECT_EQ(acquire(later), capture::CaptureStatus::FATAL_ERROR);
    EXPECT_EQ(m_callCount(FAKE_CAPTURE_HAL_ACQUIRE_CURRENT_FRAME), kAcquiresSoFar);

    // the earlier frame keeps its lock, so its release still works (tested with release in 3.1)
    EXPECT_TRUE(m_runtime->isSlotLocked(earlier.slotIndex));
}
