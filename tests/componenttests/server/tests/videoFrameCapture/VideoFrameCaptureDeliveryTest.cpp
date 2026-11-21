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

#include "FakeCaptureHalControl.h"
#include "VideoFrameCaptureFixture.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace firebolt::rialto::server::ct
{
namespace
{
constexpr std::uint64_t kFakeFrameBytes{1920ULL * 1088 + 1920ULL * 1088 / 2}; // NV12, as the fake HAL exports

// Open descriptors in this process of frames the fake HAL exported, on the server and on the client side.
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

bool isFakeFrameFile(int fd)
{
    char link[256]{};
    const std::string kPath{"/proc/self/fd/" + std::to_string(fd)};
    const ssize_t kLength = readlink(kPath.c_str(), link, sizeof(link) - 1);
    return kLength > 0 && std::string{link, static_cast<std::size_t>(kLength)}.rfind("/memfd:fake-capture-frame", 0) == 0;
}
} // namespace

/**
 * Frame delivery (R9) end to end: a committed pipeline, the fake capture HAL producing frames with real
 * descriptors, and the IPC carrying them to the client stub.
 */
class VideoFrameCaptureDeliveryTest : public VideoFrameCaptureFixture
{
public:
    ~VideoFrameCaptureDeliveryTest() override
    {
        for (int fd : m_clientFds)
            close(fd);
    }

    void commitWithCapture()
    {
        m_captureId = createCapture();
        createSession();
        loadAndCommit();
        void *hal = dlopen(FAKE_CAPTURE_HAL_PATH, RTLD_NOW | RTLD_NOLOAD);
        ASSERT_NE(hal, nullptr);
        m_pushResult = reinterpret_cast<FakeCaptureHalPushResultFunction>(dlsym(hal, "fakeCaptureHalPushResult"));
        m_callCount = reinterpret_cast<FakeCaptureHalCallCountFunction>(dlsym(hal, "fakeCaptureHalCallCount"));
        dlclose(hal); // the server keeps it loaded
        ASSERT_TRUE(m_pushResult && m_callCount);
    }

    void tearDownCommittedPipeline()
    {
        captureSinkWillBeDropped();
        gstPlayerWillBeDestructed();
        destroySession();
        destroyCapture(m_captureId);
    }

    AcquireCurrentVideoFrameResponse acquire()
    {
        AcquireCurrentVideoFrameRequest request;
        request.set_capture_id(m_captureId);
        AcquireCurrentVideoFrameResponse received;
        ConfigureAction<AcquireVideoFrame>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const AcquireCurrentVideoFrameResponse &response) { received = response; });
        for (const auto &kExported : received.exported_dma_bufs())
            m_clientFds.push_back(kExported.fd()); // the client owns what it received
        return received;
    }

    VideoFrameCaptureStatus release(const AcquireCurrentVideoFrameResponse &frame)
    {
        ReleaseVideoFrameRequest request;
        request.set_capture_id(m_captureId);
        request.set_pool_id(frame.pool_id());
        request.set_slot_index(frame.slot_index());
        request.set_acquisition_token(frame.acquisition_token());
        VideoFrameCaptureStatus status{VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR};
        ConfigureAction<ReleaseVideoFrame>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const ReleaseVideoFrameResponse &response) { status = response.status(); });
        return status;
    }

    void closeClientFds()
    {
        for (int fd : m_clientFds)
            close(fd);
        m_clientFds.clear();
    }

    std::vector<int> m_clientFds;
    FakeCaptureHalPushResultFunction m_pushResult{nullptr};
    FakeCaptureHalCallCountFunction m_callCount{nullptr};
};

/*
 * Component Test: Acquire and release frames
 * Test Objective:
 *  Check acquisition results and descriptor ownership end to end.
 *
 * Test Steps:
 *  Step 1: Before commit, acquisition answers NO_FRAME.
 *  Step 2: After commit, an acquisition answers OK with one descriptor per object and the pool description.
 *   The client's descriptor is a valid copy of the frame's file; the server's copy is closed.
 *  Step 3: NO_NEW_FRAME and NO_FRAME from the HAL are passed on, without descriptors.
 *  Step 4: The frame is released once; the client closes its descriptor and none is left.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureDeliveryTest, shouldAcquireAndReleaseFramesWithOwnedDescriptors)
{
    // Step 1: no session before commit
    m_captureId = createCapture();
    {
        AcquireCurrentVideoFrameRequest request;
        request.set_capture_id(m_captureId);
        ConfigureAction<AcquireVideoFrame>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([](const AcquireCurrentVideoFrameResponse &response)
                           { EXPECT_EQ(response.status(), VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME); });
    }
    destroyCapture(m_captureId);

    commitWithCapture();
    const int kBaseline{openFrameFiles()};

    // Step 2: a frame
    const auto kFrame = acquire();
    ASSERT_EQ(kFrame.status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
    EXPECT_EQ(kFrame.pool_id(), kStandInPoolId);
    EXPECT_EQ(kFrame.exported_dma_bufs_size(), kFrame.frame_metadata().dma_buf_objects_size());
    ASSERT_EQ(kFrame.exported_dma_bufs_size(), 1);
    ASSERT_TRUE(kFrame.has_pool_metadata());
    EXPECT_EQ(kFrame.pool_metadata().slots_size(), static_cast<int>(kStandInSlotCount));
    const int kClientFd{kFrame.exported_dma_bufs(0).fd()};
    ASSERT_GE(kClientFd, 0);
    EXPECT_TRUE(isFakeFrameFile(kClientFd));
    struct stat info
    {
    };
    ASSERT_EQ(fstat(kClientFd, &info), 0);
    EXPECT_EQ(static_cast<std::uint64_t>(info.st_size), kFakeFrameBytes);
    EXPECT_EQ(openFrameFiles(), kBaseline + 1); // the client's copy only: the server closed its own

    // Step 3: no new frame, no frame
    m_pushResult(FAKE_CAPTURE_HAL_ACQUIRE_CURRENT_FRAME, static_cast<uint32_t>(capture::CaptureStatus::NO_NEW_FRAME));
    const auto kNoNewFrame = acquire();
    EXPECT_EQ(kNoNewFrame.status(), VIDEO_FRAME_CAPTURE_STATUS_NO_NEW_FRAME);
    EXPECT_EQ(kNoNewFrame.exported_dma_bufs_size(), 0);
    m_pushResult(FAKE_CAPTURE_HAL_ACQUIRE_CURRENT_FRAME, static_cast<uint32_t>(capture::CaptureStatus::NO_FRAME));
    EXPECT_EQ(acquire().status(), VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME);

    // Step 4: release and close
    EXPECT_EQ(release(kFrame), VIDEO_FRAME_CAPTURE_STATUS_OK);
    EXPECT_EQ(release(kFrame), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT); // once only
    EXPECT_EQ(m_callCount(FAKE_CAPTURE_HAL_RELEASE_FRAME), 1);
    closeClientFds();
    EXPECT_EQ(openFrameFiles(), kBaseline);

    tearDownCommittedPipeline();
}

/*
 * Component Test: A reply that cannot be delivered
 * Test Objective:
 *  Check that a frame whose reply cannot be sent is released to the HAL exactly once and leaks no descriptor.
 *
 * Test Steps:
 *  Step 1: The client sends an acquisition; the fake HAL holds it.
 *  Step 2: The client disconnects; the HAL continues, so the server's reply cannot be written.
 *   Expect one HAL release for the frame and no open descriptor of it.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureDeliveryTest, shouldReleaseAFrameWhoseReplyCannotBeSent)
{
    commitWithCapture();
    void *hal = dlopen(FAKE_CAPTURE_HAL_PATH, RTLD_NOW | RTLD_NOLOAD);
    ASSERT_NE(hal, nullptr);
    auto blockNext =
        reinterpret_cast<FakeCaptureHalBlockNextAcquireFunction>(dlsym(hal, "fakeCaptureHalBlockNextAcquire"));
    auto waitBlocked = reinterpret_cast<FakeCaptureHalWaitUntilAcquireBlockedFunction>(
        dlsym(hal, "fakeCaptureHalWaitUntilAcquireBlocked"));
    auto unblock = reinterpret_cast<FakeCaptureHalUnblockAcquireFunction>(dlsym(hal, "fakeCaptureHalUnblockAcquire"));
    dlclose(hal);
    ASSERT_TRUE(blockNext && waitBlocked && unblock);
    const int kBaseline{openFrameFiles()};

    // Step 1: the acquisition is held inside the HAL
    blockNext();
    std::thread requester{[this]()
                          {
                              AcquireCurrentVideoFrameRequest request;
                              request.set_capture_id(m_captureId);
                              ConfigureAction<AcquireVideoFrame>(m_clientStub).send(request); // fails: disconnected
                          }};
    ASSERT_TRUE(waitBlocked(2000));

    // Step 2: the client goes before the reply; disconnecting destroys its pipeline
    captureSinkWillBeDropped();
    gstPlayerWillBeDestructed();
    m_clientStub.disconnect();
    unblock();
    requester.join();

    for (int i = 0; i < 200 && m_callCount(FAKE_CAPTURE_HAL_RELEASE_FRAME) < 1; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    EXPECT_EQ(m_callCount(FAKE_CAPTURE_HAL_RELEASE_FRAME), 1); // released once, when the send failed
    for (int i = 0; i < 200 && openFrameFiles() != kBaseline; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    EXPECT_EQ(openFrameFiles(), kBaseline); // the server closed its copy; the client got none
}

/*
 * Component Test: Concurrent acquisitions and releases
 * Test Objective:
 *  Check that the server serializes every HAL call on a session while several threads acquire and release, and
 *  that every acquired frame is released exactly once.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureDeliveryTest, shouldSerializeConcurrentAcquisitionsAndReleases)
{
    commitWithCapture();
    void *hal = dlopen(FAKE_CAPTURE_HAL_PATH, RTLD_NOW | RTLD_NOLOAD);
    ASSERT_NE(hal, nullptr);
    auto overlapDetected =
        reinterpret_cast<FakeCaptureHalOverlapDetectedFunction>(dlsym(hal, "fakeCaptureHalOverlapDetected"));
    dlclose(hal);
    ASSERT_TRUE(overlapDetected);
    const int kReleasesBefore{m_callCount(FAKE_CAPTURE_HAL_RELEASE_FRAME)};

    constexpr int kThreads{4};
    constexpr int kIterations{25};
    std::atomic<int> acquired{0};
    std::atomic<int> released{0};
    std::vector<std::thread> workers;
    for (int worker = 0; worker < kThreads; ++worker)
    {
        workers.emplace_back(
            [&]()
            {
                for (int i = 0; i < kIterations; ++i)
                {
                    AcquireCurrentVideoFrameRequest request;
                    request.set_capture_id(m_captureId);
                    AcquireCurrentVideoFrameResponse frame;
                    ConfigureAction<AcquireVideoFrame>(m_clientStub)
                        .send(request)
                        .matchResponse([&](const AcquireCurrentVideoFrameResponse &response) { frame = response; });
                    for (const auto &kExported : frame.exported_dma_bufs())
                        close(kExported.fd());
                    if (frame.status() != VIDEO_FRAME_CAPTURE_STATUS_OK)
                        continue; // NO_NEW_FRAME while every slot is held
                    ++acquired;
                    ReleaseVideoFrameRequest releaseRequest;
                    releaseRequest.set_capture_id(m_captureId);
                    releaseRequest.set_pool_id(frame.pool_id());
                    releaseRequest.set_slot_index(frame.slot_index());
                    releaseRequest.set_acquisition_token(frame.acquisition_token());
                    ConfigureAction<ReleaseVideoFrame>(m_clientStub)
                        .send(releaseRequest)
                        .matchResponse(
                            [&](const ReleaseVideoFrameResponse &response)
                            {
                                if (response.status() == VIDEO_FRAME_CAPTURE_STATUS_OK)
                                    ++released;
                            });
                }
            });
    }
    for (auto &worker : workers)
        worker.join();

    EXPECT_GT(acquired.load(), 0);
    EXPECT_EQ(released.load(), acquired.load()); // every frame released once
    EXPECT_EQ(m_callCount(FAKE_CAPTURE_HAL_RELEASE_FRAME) - kReleasesBefore, acquired.load());
    EXPECT_FALSE(overlapDetected());
    EXPECT_EQ(openFrameFiles(), 0);

    tearDownCommittedPipeline();
}
} // namespace firebolt::rialto::server::ct
