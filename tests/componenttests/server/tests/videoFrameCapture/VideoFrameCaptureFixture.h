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

#ifndef FIREBOLT_RIALTO_SERVER_CT_VIDEO_FRAME_CAPTURE_FIXTURE_H_
#define FIREBOLT_RIALTO_SERVER_CT_VIDEO_FRAME_CAPTURE_FIXTURE_H_

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ExpectMessage.h"
#include "ICaptureAdmission.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

#include <dlfcn.h>

#include <cstdint>
#include <cstdlib>

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::NotNull;
using testing::Return;
using testing::SetArgPointee;
using testing::StrEq;

namespace
{
constexpr const char *kCaptureHalLibraryEnv{"RIALTO_CAPTURE_HAL_LIBRARY"};
constexpr std::uint32_t kStandInPoolId{1};
constexpr std::uint32_t kStandInSlotCount{3};
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCountInPlayingState{3};
constexpr int kPositionInPaused{10};
constexpr double kSeekPlaybackRate{1.0};
const std::string kVideoSinkTypeName{"VideoSink"};
} // namespace

namespace firebolt::rialto::server::ct
{
/**
 * Video frame capture component tests: the fake capture HAL is loaded, and a stand-in for pool admission (R8)
 * accepts or refuses configured sessions. The helpers mock the capture sink set-up of a commit.
 */
class VideoFrameCaptureFixture : public MediaPipelineTest
{
public:
    VideoFrameCaptureFixture()
    {
        // the capture HAL is loaded on the first create
        setenv(kCaptureHalLibraryEnv, FAKE_CAPTURE_HAL_PATH, 1);
        // pool admission (R8) does not exist yet: these tests accept every configured session
        m_defaultAdmissionFactory = ICaptureAdmissionFactory::instance();
        ICaptureAdmissionFactory::instance() = std::make_shared<AcceptingAdmissionFactory>(m_admissionAccepts);
        m_captureSink = gst_element_factory_make("fakesink", nullptr);
        m_sinkFactory = gst_element_factory_find("fakesink"); // a real object: the server casts it
    }

    ~VideoFrameCaptureFixture() override
    {
        gst_object_unref(m_sinkFactory);
        gst_object_unref(m_captureSink);
        ICaptureAdmissionFactory::instance() = m_defaultAdmissionFactory;
        unsetenv(kCaptureHalLibraryEnv);
    }

    // Stands in for pool admission (R8): accepts or refuses as the test sets.
    class AcceptingAdmission : public ICaptureAdmission
    {
    public:
        explicit AcceptingAdmission(const std::shared_ptr<bool> &accepts) : m_accepts{accepts} {}
        // like R8, an accepted session gets a pool on the HAL
        bool admit(CaptureSessionHandle &handle) override
        {
            if (!handle.session || !*m_accepts)
                return false;
            handle.poolId = kStandInPoolId;
            return handle.session->createPool(kStandInSlotCount, handle.pool) == capture::CaptureStatus::OK;
        }

    private:
        std::shared_ptr<bool> m_accepts;
    };
    class AcceptingAdmissionFactory : public ICaptureAdmissionFactory
    {
    public:
        explicit AcceptingAdmissionFactory(const std::shared_ptr<bool> &accepts) : m_accepts{accepts} {}
        std::shared_ptr<ICaptureAdmission> createCaptureAdmission() const override
        {
            return std::make_shared<AcceptingAdmission>(m_accepts);
        }

    private:
        std::shared_ptr<bool> m_accepts;
    };

    // The commit with a capture builds the codec caps, then creates, configures and readies a westerossink.
    void captureSinkWillBeConfigured()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq("video/x-h264")))
            .WillOnce(Return(&m_captureCaps))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_captureCaps, StrEq("alignment"), G_TYPE_STRING, StrEq("nal")));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_captureCaps, StrEq("stream-format"), G_TYPE_STRING, StrEq("raw")));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_captureCaps, StrEq("width"), G_TYPE_INT, kWidth));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_captureCaps, StrEq("height"), G_TYPE_INT, kHeight));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink"))).WillOnce(Return(m_sinkFactory));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(m_sinkFactory, nullptr)).WillOnce(Return(m_captureSink));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_sinkFactory)));
        for (const char *kName : {"rdk-capture-session", "codec", "resolution"})
            EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq(kName))).WillOnce(Return(&m_paramSpec));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetPointerStub(m_captureSink, StrEq("rdk-capture-session"), NotNull()));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetPointerStub(m_captureSink, StrEq("codec"), &m_captureCaps));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStrStub(m_captureSink, StrEq("resolution"), StrEq("1920x1080")));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_captureSink));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink")));
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_captureSink, GST_STATE_READY))
            .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(m_captureSink, _, _, _))
            .WillOnce(DoAll(SetArgPointee<1>(GST_STATE_READY), Return(GST_STATE_CHANGE_SUCCESS)));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_captureCaps));
    }

    // The configuration up to the READY request; the outcome of READY is left to the caller.
    void captureSinkWillBeSetUpUntilReady(const std::string &resolution = "1920x1080")
    {
        EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq("video/x-h264")))
            .WillOnce(Return(&m_captureCaps))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_captureCaps, StrEq("alignment"), G_TYPE_STRING, StrEq("nal")));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_captureCaps, StrEq("stream-format"), G_TYPE_STRING, StrEq("raw")));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_captureCaps, StrEq("width"), G_TYPE_INT, kWidth));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_captureCaps, StrEq("height"), G_TYPE_INT, kHeight));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink")))
            .WillOnce(Return(m_sinkFactory))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(m_sinkFactory, nullptr))
            .WillOnce(Return(m_captureSink))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_sinkFactory))).RetiresOnSaturation();
        for (const char *kName : {"rdk-capture-session", "codec", "resolution"})
            EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq(kName))).WillOnce(Return(&m_paramSpec));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetPointerStub(m_captureSink, StrEq("rdk-capture-session"), NotNull()));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetPointerStub(m_captureSink, StrEq("codec"), &m_captureCaps));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStrStub(m_captureSink, StrEq("resolution"), StrEq(resolution)));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_captureSink));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_captureCaps));
    }

    // Rollback of the capture sink: NULL, video-sink cleared, reference dropped.
    void captureSinkWillBeDiscarded()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_captureSink, GST_STATE_NULL))
            .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_captureSink)));
    }

    // A loaded pipeline with the capture attached and the given sources set up, not yet committed.
    void prepareCommit(bool withVideo)
    {
        gstPlayerWillBeCreated();
        load();
        attach(m_captureId, true);
        audioSourceWillBeAttached();
        attachAudioSource();
        if (withVideo)
        {
            videoSourceWillBeAttached();
            attachVideoSource();
        }
        sourceWillBeSetup();
        setupSource();
    }

    void commitShouldFail()
    {
        ConfigureAction<AllSourcesAttached>(m_clientStub).send(createAllSourcesAttachedRequest(m_sessionId)).expectFailure();
    }

    // Live HAL sessions in the fake capture HAL, which the server has loaded by now.
    int liveHalSessions()
    {
        void *hal = dlopen(FAKE_CAPTURE_HAL_PATH, RTLD_NOW | RTLD_NOLOAD);
        if (!hal)
            return -1;
        auto liveSessions = reinterpret_cast<int (*)()>(dlsym(hal, "fakeCaptureHalLiveSessions"));
        const int kLive{liveSessions ? liveSessions() : -1};
        dlclose(hal);
        return kLive;
    }

    // Destroying a committed pipeline drops the player's reference to its capture sink.
    void captureSinkWillBeDropped() { EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_captureSink))); }

    std::shared_ptr<ICaptureAdmissionFactory> m_defaultAdmissionFactory;
    std::shared_ptr<bool> m_admissionAccepts{std::make_shared<bool>(true)};
    GstElement *m_captureSink{nullptr};
    GstCaps m_captureCaps{};
    GstElementFactory *m_sinkFactory{nullptr};
    GParamSpec m_paramSpec{};

    std::uint32_t createCapture()
    {
        CreateVideoFrameCaptureRequest request;
        request.set_api(GRAPHICS_API_TYPE_GL);
        std::uint32_t captureId{0};
        ConfigureAction<CreateVideoFrameCapture>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const CreateVideoFrameCaptureResponse &response) { captureId = response.capture_id(); });
        EXPECT_NE(captureId, 0U);
        return captureId;
    }

    void attach(std::uint32_t captureId, bool expectSuccess)
    {
        AttachVideoFrameCaptureRequest request;
        request.set_session_id(m_sessionId);
        request.set_capture_id(captureId);
        auto action = ConfigureAction<AttachVideoFrameCapture>(m_clientStub);
        action.send(request);
        expectSuccess ? action.expectSuccess() : action.expectFailure();
    }

    void detach(bool expectSuccess)
    {
        DetachVideoFrameCaptureRequest request;
        request.set_session_id(m_sessionId);
        auto action = ConfigureAction<DetachVideoFrameCapture>(m_clientStub);
        action.send(request);
        expectSuccess ? action.expectSuccess() : action.expectFailure();
    }

    // In a state after commit: a new capture is refused, and detach succeeds only in STOPPED.
    void checkCommittedState(bool detachAllowed)
    {
        attach(m_otherCaptureId, false);
        if (!detachAllowed)
            detach(false);
    }

    void loadAndCommit()
    {
        gstPlayerWillBeCreated();
        load();
        attach(m_captureId, true); // attached before commit, sources attached after it
        audioSourceWillBeAttached();
        attachAudioSource();
        videoSourceWillBeAttached();
        attachVideoSource();
        sourceWillBeSetup();
        setupSource();
        willSetupAndAddSource(&m_audioAppSrc);
        willSetupAndAddSource(&m_videoAppSrc);
        willFinishSetupAndAddSource();
        captureSinkWillBeConfigured();
        indicateAllSourcesAttached({&m_audioAppSrc, &m_videoAppSrc});
    }

    void pauseWithPreroll()
    {
        willPause();
        pause();
        {
            ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};
            pushAudioData(kFramesToPush, kPrerollNumFrames);
            pushVideoData(kFramesToPush, kPrerollNumFrames);
            ASSERT_TRUE(expectedNetworkStateChange.getMessage());
        }
        willNotifyPaused();
        notifyPaused();
    }

    void destroyCapture(std::uint32_t captureId)
    {
        DestroyVideoFrameCaptureRequest request;
        request.set_capture_id(captureId);
        ConfigureAction<DestroyVideoFrameCapture>(m_clientStub).send(request).expectSuccess();
    }

    void createAndLoadSession()
    {
        createSession();
        gstPlayerWillBeCreated();
        load();
    }

    void destroyLoadedSession()
    {
        gstPlayerWillBeDestructed();
        destroySession();
    }

    std::uint32_t m_captureId{0};
    std::uint32_t m_otherCaptureId{0};
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_VIDEO_FRAME_CAPTURE_FIXTURE_H_
