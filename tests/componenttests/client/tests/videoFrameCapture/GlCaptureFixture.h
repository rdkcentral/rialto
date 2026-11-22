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

#ifndef FIREBOLT_RIALTO_CLIENT_CT_GL_CAPTURE_FIXTURE_H_
#define FIREBOLT_RIALTO_CLIENT_CT_GL_CAPTURE_FIXTURE_H_

#include "ClientComponentTest.h"
#include "IIpcController.h"
#include "IVideoFrameCapture.h"
#include <gtest/gtest.h>

#include <dirent.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

namespace firebolt::rialto::client::ct
{
namespace
{
constexpr std::uint32_t kCaptureId{3};
constexpr std::uint32_t kPoolId{1};
constexpr std::uint32_t kNv12{0x3231564e};
constexpr const char *kMemfdName{"vfc-gl-ct"};

// Open descriptors, in this process, of memfds this test created: the server stub's and the client's copies.
int openTestMemfds()
{
    int count{0};
    DIR *dir = opendir("/proc/self/fd");
    if (!dir)
        return -1;
    const std::string kTarget{std::string{"/memfd:"} + kMemfdName};
    while (dirent *entry = readdir(dir))
    {
        char link[256]{};
        const std::string kPath{std::string{"/proc/self/fd/"} + entry->d_name};
        const ssize_t kLength = readlink(kPath.c_str(), link, sizeof(link) - 1);
        if (kLength > 0 && std::string{link, static_cast<std::size_t>(kLength)}.rfind(kTarget, 0) == 0)
            ++count;
    }
    closedir(dir);
    return count;
}
} // namespace

/**
 * A GL capture end to end in the client: the server stub answers acquisitions with real DMA-BUF stand-ins (memfds)
 * sent through the IPC, and frames are imported through the fake libEGL, which counts live GL objects.
 */
class GlCaptureFixture : public ClientComponentTest
{
public:
    GlCaptureFixture() : ClientComponentTest()
    {
        enableVideoFrameCaptureModule();
        ClientComponentTest::startApplicationRunning();
    }

    ~GlCaptureFixture()
    {
        if (m_fakeEgl)
            dlclose(m_fakeEgl);
        ClientComponentTest::stopApplication();
    }

    std::shared_ptr<IGLVideoFrameCapture> createCapture()
    {
        EXPECT_CALL(*m_videoFrameCaptureModuleMock, createVideoFrameCapture(_, _, _, _))
            .WillOnce(Invoke(
                [](::google::protobuf::RpcController *, const CreateVideoFrameCaptureRequest *,
                   CreateVideoFrameCaptureResponse *response, ::google::protobuf::Closure *done)
                {
                    response->set_status(VIDEO_FRAME_CAPTURE_STATUS_OK);
                    response->set_capture_id(kCaptureId);
                    done->Run();
                }));
        auto capture = IVideoFrameCaptureFactory::createFactory()->createGLVideoFrameCapture(m_display, m_context);

        // the capture loaded the fake libEGL; reach its control functions
        m_fakeEgl = dlopen(FAKE_EGL_PATH, RTLD_NOW | RTLD_NOLOAD);
        EXPECT_NE(m_fakeEgl, nullptr);
        if (m_fakeEgl)
        {
            m_liveTextures = reinterpret_cast<int (*)()>(dlsym(m_fakeEgl, "fakeEglLiveTextures"));
            m_liveImages = reinterpret_cast<int (*)()>(dlsym(m_fakeEgl, "fakeEglLiveImages"));
            m_liveContexts = reinterpret_cast<int (*)()>(dlsym(m_fakeEgl, "fakeEglLiveContexts"));
            m_threadCalls = reinterpret_cast<int (*)()>(dlsym(m_fakeEgl, "fakeEglThreadCallCount"));
            m_resetThreadCalls = reinterpret_cast<void (*)()>(dlsym(m_fakeEgl, "fakeEglResetThreadCallCount"));
            auto reset = reinterpret_cast<void (*)()>(dlsym(m_fakeEgl, "fakeEglReset"));
            m_makeCurrent =
                reinterpret_cast<unsigned (*)(void *, void *, void *, void *)>(dlsym(m_fakeEgl, "eglMakeCurrent"));
            if (reset)
                reset();
        }
        return capture;
    }

    // The application's context is current on this thread, as it is when it acquires.
    void makeApplicationContextCurrent() { m_makeCurrent(m_display, nullptr, nullptr, m_context); }

    // A frame in \a slot; its one object is 101 + slot unless \a objectId is given. A \a matrix (wire value)
    // adds a colour description with narrow range, co-sited horizontally and midpoint vertically.
    void serverWillReturnFrame(std::uint32_t slot, std::uint64_t token, bool flushed, std::uint64_t objectId = 0,
                               int matrix = -1)
    {
        EXPECT_CALL(*m_videoFrameCaptureModuleMock, acquireCurrentFrame(_, _, _, _))
            .WillOnce(Invoke(
                [slot, token, flushed, objectId,
                 matrix](::google::protobuf::RpcController *controller, const AcquireCurrentVideoFrameRequest *request,
                         AcquireCurrentVideoFrameResponse *response, ::google::protobuf::Closure *done)
                {
                    EXPECT_EQ(request->capture_id(), kCaptureId);
                    response->set_status(VIDEO_FRAME_CAPTURE_STATUS_OK);
                    response->set_pool_id(kPoolId);
                    response->set_slot_index(slot);
                    response->set_acquisition_token(token);
                    response->set_dma_buf_objects_flushed(flushed);
                    response->mutable_visible_region()->set_width(64);
                    response->mutable_visible_region()->set_height(32);
                    auto *metadata = response->mutable_frame_metadata();
                    metadata->set_drm_format(kNv12);
                    metadata->set_backing_width(64);
                    metadata->set_backing_height(32);
                    auto *object = metadata->add_dma_buf_objects();
                    object->set_object_id(objectId ? objectId : 101 + slot);
                    object->set_size_bytes(4096);
                    for (std::uint32_t index = 0; index < 2; ++index)
                    {
                        auto *plane = metadata->add_planes();
                        plane->set_plane_index(index);
                        plane->set_dma_buf_object_id(objectId ? objectId : 101 + slot);
                        plane->set_offset_bytes(index * 2048);
                        plane->set_length_bytes(2048);
                        plane->set_stride_bytes(64);
                    }
                    if (matrix >= 0)
                    {
                        auto *colour = metadata->mutable_color_description();
                        colour->set_ycbcr_color_space(static_cast<VideoFrameCaptureYcbcrColorSpace>(matrix));
                        colour->set_sample_range(VIDEO_FRAME_CAPTURE_SAMPLE_RANGE_NARROW);
                        colour->set_horizontal_chroma_siting(VIDEO_FRAME_CAPTURE_CHROMA_SITING_0);
                        colour->set_vertical_chroma_siting(VIDEO_FRAME_CAPTURE_CHROMA_SITING_0_5);
                    }
                    auto *exported = response->add_exported_dma_bufs();
                    exported->set_dma_buf_object_id(objectId ? objectId : 101 + slot);
                    exported->set_fd(memfd_create(kMemfdName, MFD_CLOEXEC));
                    auto *pool = response->mutable_pool_metadata();
                    pool->set_pool_id(kPoolId);
                    for (int i = 0; i < 3; ++i)
                        pool->add_slots();
                    // the stub's descriptor is closed by the IPC once the reply is sent, as the server does
                    if (auto *ipcController = dynamic_cast<::firebolt::rialto::ipc::IController *>(controller))
                        ipcController->setReplyFdsOwned(true);
                    done->Run();
                }))
            .RetiresOnSaturation();
    }

    void serverWillReturnNoNewFrame()
    {
        EXPECT_CALL(*m_videoFrameCaptureModuleMock, acquireCurrentFrame(_, _, _, _))
            .WillOnce(Invoke(
                [](::google::protobuf::RpcController *, const AcquireCurrentVideoFrameRequest *,
                   AcquireCurrentVideoFrameResponse *response, ::google::protobuf::Closure *done)
                {
                    response->set_status(VIDEO_FRAME_CAPTURE_STATUS_NO_NEW_FRAME);
                    done->Run();
                }))
            .RetiresOnSaturation();
    }

    void serverWillRelease(std::uint32_t slot, std::uint64_t token)
    {
        EXPECT_CALL(*m_videoFrameCaptureModuleMock, releaseFrame(_, _, _, _))
            .WillOnce(Invoke(
                [slot, token](::google::protobuf::RpcController *, const ReleaseVideoFrameRequest *request,
                              ReleaseVideoFrameResponse *response, ::google::protobuf::Closure *done)
                {
                    EXPECT_EQ(request->capture_id(), kCaptureId);
                    EXPECT_EQ(request->pool_id(), kPoolId);
                    EXPECT_EQ(request->slot_index(), slot);
                    EXPECT_EQ(request->acquisition_token(), token);
                    response->set_status(VIDEO_FRAME_CAPTURE_STATUS_OK);
                    done->Run();
                }))
            .RetiresOnSaturation();
    }

    void serverWillDestroyCapture()
    {
        EXPECT_CALL(*m_videoFrameCaptureModuleMock, destroyVideoFrameCapture(_, _, _, _))
            .WillOnce(Invoke([](::google::protobuf::RpcController *, const DestroyVideoFrameCaptureRequest *,
                                DestroyVideoFrameCaptureResponse *, ::google::protobuf::Closure *done) { done->Run(); }));
    }

    EGLDisplay m_display{reinterpret_cast<EGLDisplay>(0xD1)};
    EGLContext m_context{reinterpret_cast<EGLContext>(0xC1)};
    void *m_fakeEgl{nullptr};
    int (*m_liveTextures)(){nullptr};
    int (*m_liveImages)(){nullptr};
    int (*m_liveContexts)(){nullptr};
    int (*m_threadCalls)(){nullptr};
    void (*m_resetThreadCalls)(){nullptr};
    unsigned (*m_makeCurrent)(void *, void *, void *, void *){nullptr};
};
} // namespace firebolt::rialto::client::ct

#endif // FIREBOLT_RIALTO_CLIENT_CT_GL_CAPTURE_FIXTURE_H_
