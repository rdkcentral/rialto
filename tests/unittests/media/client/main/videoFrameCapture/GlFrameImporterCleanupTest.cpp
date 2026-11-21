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

#include "CaptureCore.h"
#include "EglLibrary.h"
#include "FakeEglControl.h"
#include "GlFrameImporter.h"
#include "VideoFrameCaptureIpcMock.h"

#include <gtest/gtest.h>

#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdlib>
#include <thread>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

/**
 * Runs the GL importer against the fake libEGL, whose per-thread call counter shows which thread made GL calls.
 */
class GlFrameImporterCleanupTest : public ::testing::Test
{
protected:
    static constexpr std::uint32_t kNv12{0x3231564e};

    void SetUp() override
    {
        setenv(EglLibrary::kLibraryEnv, FAKE_EGL_PATH, 1);
        m_gl = m_library.functions();
        ASSERT_NE(m_gl, nullptr);
        m_fake = dlopen(FAKE_EGL_PATH, RTLD_NOW | RTLD_NOLOAD);
        ASSERT_NE(m_fake, nullptr);
        m_reset = reinterpret_cast<void (*)()>(dlsym(m_fake, "fakeEglReset"));
        m_liveTextures = reinterpret_cast<int (*)()>(dlsym(m_fake, "fakeEglLiveTextures"));
        m_liveImages = reinterpret_cast<int (*)()>(dlsym(m_fake, "fakeEglLiveImages"));
        m_threadCalls = reinterpret_cast<int (*)()>(dlsym(m_fake, "fakeEglThreadCallCount"));
        m_resetThreadCalls = reinterpret_cast<void (*)()>(dlsym(m_fake, "fakeEglResetThreadCallCount"));
        ASSERT_TRUE(m_reset && m_liveTextures && m_liveImages && m_threadCalls && m_resetThreadCalls);
        m_reset();

        ON_CALL(*m_ipc, releaseFrame(_, _, _, _)).WillByDefault(Return(true));
        m_core = std::make_shared<CaptureCore>(m_ipc, 5, std::make_shared<GlFrameImporter>(m_gl, m_display, m_context));
    }

    void TearDown() override
    {
        m_core.reset();
        if (m_fake)
            dlclose(m_fake);
        unsetenv(EglLibrary::kLibraryEnv);
    }

    CaptureAcquireResult frame(std::uint32_t slot, std::uint64_t token, bool flushed)
    {
        CaptureAcquireResult result;
        result.status = CaptureAcquireStatus::OK;
        result.description.slotIndex = slot;
        result.description.acquisitionToken = token;
        result.description.drmFormat = kNv12;
        result.description.backingWidth = 64;
        result.description.backingHeight = 32;
        result.description.dmaBufObjectsFlushed = flushed;
        result.description.objects = {{101, 4096}};
        result.description.planes = {{0, 101, 0, 2048, 64}, {1, 101, 2048, 1024, 64}};
        result.fds = {{101, memfd_create("gl-cleanup-test", MFD_CLOEXEC)}};
        result.pool = CapturePoolDescription{1, 3};
        return result;
    }

    static CaptureAcquireResult noNewFrame()
    {
        CaptureAcquireResult result;
        result.status = CaptureAcquireStatus::NO_NEW_FRAME;
        return result;
    }

    EglLibrary m_library;
    std::shared_ptr<IEglGl> m_gl;
    void *m_fake{nullptr};
    void (*m_reset)(){nullptr};
    int (*m_liveTextures)(){nullptr};
    int (*m_liveImages)(){nullptr};
    int (*m_threadCalls)(){nullptr};
    void (*m_resetThreadCalls)(){nullptr};
    egl::EGLDisplay m_display{reinterpret_cast<egl::EGLDisplay>(0xD1)};
    egl::EGLContext m_context{reinterpret_cast<egl::EGLContext>(0xC1)};
    std::shared_ptr<NiceMock<VideoFrameCaptureIpcMock>> m_ipc{std::make_shared<NiceMock<VideoFrameCaptureIpcMock>>()};
    std::shared_ptr<CaptureCore> m_core;
};

TEST_F(GlFrameImporterCleanupTest, ReleaseOnAnotherThreadMakesNoGlCallsAndTheNextAcquireDeletes)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(5))
        .WillOnce(Return(frame(0, 1, false)))
        .WillOnce(Return(frame(1, 2, true))) // invalidates the import the first frame still uses
        .WillOnce(Return(noNewFrame()));

    std::shared_ptr<IGLVideoFrame> held = m_core->acquireCurrentFrame().frame;
    ASSERT_NE(held, nullptr);
    ASSERT_NE(m_core->acquireCurrentFrame().frame, nullptr);
    EXPECT_EQ(m_liveTextures(), 2); // the retired import waits for its frame

    // the application drops the old frame on a thread with no context
    int callsOnDroppingThread{-1};
    std::thread dropper{[&]()
                        {
                            m_resetThreadCalls();
                            held.reset();
                            callsOnDroppingThread = m_threadCalls();
                        }};
    dropper.join();
    EXPECT_EQ(callsOnDroppingThread, 0);
    EXPECT_EQ(m_liveTextures(), 2); // queued, not deleted

    m_core->acquireCurrentFrame(); // deletes the queued import
    EXPECT_EQ(m_liveTextures(), 1);
    EXPECT_EQ(m_liveImages(), 1);
}

TEST_F(GlFrameImporterCleanupTest, ImportInvalidatedByAnAcquisitionIsDeletedInThatAcquisition)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(5)).WillOnce(Return(frame(0, 1, false))).WillOnce(Return(frame(1, 2, true)));

    m_core->acquireCurrentFrame();
    m_core->acquireCurrentFrame(); // the first frame is no longer held by anyone
    EXPECT_EQ(m_liveTextures(), 1);
    EXPECT_EQ(m_liveImages(), 1);
}
