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
#include "ICaptureSession.h"
#include "VideoFrameCaptureFixture.h"

#include <dlfcn.h>
#include <unistd.h>

#include <cstdlib>
#include <vector>

namespace firebolt::rialto::server::ct
{
/**
 * Pool admission (R8) and the capabilities and pool queries, end to end: the real server with the real admission and
 * the fake capture HAL.
 */
class VideoFrameCaptureAdmissionTest : public VideoFrameCaptureFixture
{
public:
    VideoFrameCaptureAdmissionTest()
    {
        ICaptureAdmissionFactory::instance() = m_defaultAdmissionFactory; // the real admission, not the stand-in
        // held open, so the fake HAL keeps the state set here while the server loads and unloads it
        m_hal = dlopen(FAKE_CAPTURE_HAL_PATH, RTLD_NOW | RTLD_LOCAL);
        EXPECT_NE(m_hal, nullptr);
        if (m_hal)
        {
            m_reset = reinterpret_cast<FakeCaptureHalResetFunction>(dlsym(m_hal, "fakeCaptureHalReset"));
            m_setSlotLimits =
                reinterpret_cast<FakeCaptureHalSetSlotLimitsFunction>(dlsym(m_hal, "fakeCaptureHalSetSlotLimits"));
            m_pushResult = reinterpret_cast<FakeCaptureHalPushResultFunction>(dlsym(m_hal, "fakeCaptureHalPushResult"));
            m_callCount = reinterpret_cast<FakeCaptureHalCallCountFunction>(dlsym(m_hal, "fakeCaptureHalCallCount"));
        }
        EXPECT_TRUE(m_reset && m_setSlotLimits && m_pushResult && m_callCount);
        if (m_reset)
            m_reset();
    }

    ~VideoFrameCaptureAdmissionTest() override
    {
        for (int fd : m_clientFds)
            close(fd);
        unsetenv("RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS");
        if (m_reset)
            m_reset();
        if (m_hal)
            dlclose(m_hal);
    }

    GetVideoFrameCaptureCapabilitiesResponse getCapabilities()
    {
        GetVideoFrameCaptureCapabilitiesRequest request;
        request.set_capture_id(m_captureId);
        request.set_api(GRAPHICS_API_TYPE_GL);
        GetVideoFrameCaptureCapabilitiesResponse received;
        ConfigureAction<GetVideoFrameCaptureCapabilities>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const GetVideoFrameCaptureCapabilitiesResponse &response) { received = response; });
        return received;
    }

    GetVideoFrameCapturePoolResponse getPool()
    {
        GetVideoFrameCapturePoolRequest request;
        request.set_capture_id(m_captureId);
        GetVideoFrameCapturePoolResponse received;
        ConfigureAction<GetVideoFrameCapturePool>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const GetVideoFrameCapturePoolResponse &response) { received = response; });
        return received;
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
            m_clientFds.push_back(kExported.fd());
        return received;
    }

    void release(const AcquireCurrentVideoFrameResponse &frame)
    {
        ReleaseVideoFrameRequest request;
        request.set_capture_id(m_captureId);
        request.set_pool_id(frame.pool_id());
        request.set_slot_index(frame.slot_index());
        request.set_acquisition_token(frame.acquisition_token());
        ConfigureAction<ReleaseVideoFrame>(m_clientStub).send(request).expectSuccess();
    }

    // The configured sink reaches READY, then admission refuses: the sink is discarded.
    void commitShouldBeRefusedByAdmission()
    {
        captureSinkWillBeSetUpUntilReady();
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink"))).Times(2).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_captureSink, GST_STATE_READY))
            .WillOnce(Return(GST_STATE_CHANGE_SUCCESS))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(m_captureSink, _, _, _))
            .WillOnce(DoAll(SetArgPointee<1>(GST_STATE_READY), Return(GST_STATE_CHANGE_SUCCESS)))
            .RetiresOnSaturation();
        captureSinkWillBeDiscarded();
        commitShouldFail();
    }

    // The commit, now admitted, with the shipping setup that follows it.
    void commitShouldSucceed()
    {
        willSetupAndAddSource(&m_audioAppSrc);
        willSetupAndAddSource(&m_videoAppSrc);
        willFinishSetupAndAddSource();
        captureSinkWillBeConfigured();
        indicateAllSourcesAttached({&m_audioAppSrc, &m_videoAppSrc});
    }

    void tearDown()
    {
        captureSinkWillBeDropped();
        gstPlayerWillBeDestructed();
        destroySession();
        destroyCapture(m_captureId);
    }

    void *m_hal{nullptr};
    FakeCaptureHalResetFunction m_reset{nullptr};
    FakeCaptureHalSetSlotLimitsFunction m_setSlotLimits{nullptr};
    FakeCaptureHalPushResultFunction m_pushResult{nullptr};
    FakeCaptureHalCallCountFunction m_callCount{nullptr};
    std::vector<int> m_clientFds;
};

/*
 * Component Test: Admission and the capabilities query through a playback
 * Test Objective:
 *  Check that the real admission admits the chosen slot count and that the capabilities and pool queries follow the
 *  capture's state.
 *
 * Test Steps:
 *  Step 1: Attached, not committed: getCapabilities and getPool answer SESSION_NOT_READY.
 *  Step 2: With RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS=2 and the fake HAL's [1, 4], the commit succeeds.
 *  Step 3: getCapabilities answers OK with N = 2; getPool answers the admitted pool with two slots; the first
 *          acquisition reports the same pool ID.
 *  Step 4: After stop, getCapabilities answers SESSION_NOT_READY.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAdmissionTest, shouldAdmitTheTargetSlotCountAndReportIt)
{
    setenv("RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS", "2", 1);
    m_captureId = createCapture();
    createSession();
    prepareCommit(true);

    // Step 1: not committed
    EXPECT_EQ(getCapabilities().status(), VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY);
    EXPECT_EQ(getPool().status(), VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY);

    // Step 2: committed
    commitShouldSucceed();
    EXPECT_EQ(m_callCount(FAKE_CAPTURE_HAL_CREATE_POOL), 1);

    // Step 3: the admitted session
    const auto kCapabilities = getCapabilities();
    ASSERT_EQ(kCapabilities.status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
    EXPECT_EQ(kCapabilities.max_concurrent_frames(), 2U);
    EXPECT_EQ(kCapabilities.max_resources_per_frame(), 1U);
    ASSERT_EQ(kCapabilities.drm_fourccs_size(), 1);
    EXPECT_EQ(kCapabilities.drm_fourccs(0), 0x3231564eU); // NV12

    const auto kPool = getPool();
    ASSERT_EQ(kPool.status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
    EXPECT_NE(kPool.pool_id(), 0U);
    EXPECT_EQ(kPool.slots_size(), 2);
    EXPECT_EQ(kPool.minimum_slots(), 1U);
    EXPECT_EQ(kPool.maximum_slots(), 4U);

    const auto kFrame = acquire();
    ASSERT_EQ(kFrame.status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
    EXPECT_EQ(kFrame.pool_id(), kPool.pool_id());
    release(kFrame);

    // Step 4: stopped
    willStop();
    stop();
    EXPECT_EQ(getCapabilities().status(), VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY);

    tearDown();
}

/*
 * Component Test: A single-slot HAL is refused, and a retry is admitted
 * Expected Results:
 *  With the fake HAL's limits [1, 1], allSourcesAttached() fails and no HAL session is left; with [1, 4], the retry
 *  commits with the default N = 3.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAdmissionTest, shouldRefuseASingleSlotHalAndAdmitOnRetry)
{
    m_captureId = createCapture();
    createSession();
    prepareCommit(true);

    m_setSlotLimits(1, 1);
    commitShouldBeRefusedByAdmission();
    EXPECT_EQ(liveHalSessions(), 0);
    EXPECT_EQ(m_callCount(FAKE_CAPTURE_HAL_CREATE_POOL), 0);

    m_setSlotLimits(1, 4);
    commitShouldSucceed();
    EXPECT_EQ(liveHalSessions(), 1);
    EXPECT_EQ(getCapabilities().max_concurrent_frames(), 3U);

    tearDown();
}

/*
 * Component Test: The HAL becomes ready after two "not available yet" answers
 * Expected Results:
 *  Admission retries within the READY timeout and the commit succeeds after the third query.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAdmissionTest, shouldCommitWhenTheHalBecomesReadyLate)
{
    m_captureId = createCapture();
    createSession();
    prepareCommit(true);

    const auto kNotReady{static_cast<std::uint32_t>(capture::CaptureStatus::CAPABILITIES_NOT_AVAILABLE_YET)};
    m_pushResult(FAKE_CAPTURE_HAL_GET_CAPABILITIES, kNotReady);
    m_pushResult(FAKE_CAPTURE_HAL_GET_CAPABILITIES, kNotReady);
    commitShouldSucceed();
    EXPECT_EQ(m_callCount(FAKE_CAPTURE_HAL_GET_CAPABILITIES), 3);
    EXPECT_EQ(m_callCount(FAKE_CAPTURE_HAL_CREATE_POOL), 1);

    tearDown();
}
} // namespace firebolt::rialto::server::ct
