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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Constants.h"
#include "ExpectMessage.h"
#include "GstSrc.h"
#include "GstreamerStub.h"
#include "IMediaFrameWriter.h"
#include "IMediaPipeline.h"
#include "Matchers.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"
#include "SegmentBuilder.h"
#include "ShmHandle.h"

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCountInPlayingState{24};
constexpr GType kSecondaryGstPlayFlagsType{static_cast<GType>(234)};
const std::string kDummyStateName{"dummy"};
} // namespace

using testing::_;
using testing::AtLeast;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::StrEq;

namespace firebolt::rialto::server::ct
{
class DualVideoPlaybackTest : public MediaPipelineTest
{
public:
    testing::Sequence m_writeBufferSeq;

    DualVideoPlaybackTest() = default;
    ~DualVideoPlaybackTest() override = default;

    void secondaryGstPlayerWillBeCreated()
    {
        m_secondaryGstreamerStub.setupPipeline();
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("rialtosrc")))
            .WillOnce(Return(nullptr))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(0, StrEq("rialtosrc"), _, _)).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("playbin"), _))
            .WillOnce(Return(&m_secondaryPipeline))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_glibWrapperMock, gTypeFromName(StrEq("GstPlayFlags")))
            .Times(3)
            .WillRepeatedly(Return(kSecondaryGstPlayFlagsType))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kSecondaryGstPlayFlagsType))
            .Times(3)
            .WillRepeatedly(Return(&m_flagsClass));
        EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("audio")))
            .WillOnce(Return(&m_audioFlag))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("video")))
            .WillOnce(Return(&m_videoFlag))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("native-video")))
            .WillOnce(Return(&m_nativeVideoFlag))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("brcmaudiosink")))
            .WillOnce(Return(nullptr))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_secondaryPipeline, StrEq("flags")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_secondaryPipeline, StrEq("uri")));
        EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(GST_BIN(&m_secondaryPipeline), StrEq("playsink")))
            .WillOnce(Return(&m_secondaryPlaysink));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_secondaryPlaysink, StrEq("send-event-mode")));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_secondaryPlaysink));

        // In case of longer testruns, GstPlayer may request to query position
        EXPECT_CALL(*m_gstWrapperMock, gstElementQueryPosition(&m_secondaryPipeline, GST_FORMAT_TIME, _))
            .Times(AtLeast(0))
            .WillRepeatedly(Return(false));
    }

    void secondaryVideoSourceWillBeAttached()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq("video/x-h264")))
            .WillOnce(Return(&m_videoCaps))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_videoCaps, StrEq("alignment"), G_TYPE_STRING, StrEq("nal")))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleStringStub(&m_videoCaps, StrEq("stream-format"), G_TYPE_STRING, StrEq("raw")))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_videoCaps, StrEq("width"), G_TYPE_INT, kWidth))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_videoCaps, StrEq("height"), G_TYPE_INT, kHeight))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&m_videoCaps)).WillOnce(Return(&m_videoCapsStr)).RetiresOnSaturation();
        EXPECT_CALL(*m_glibWrapperMock, gFree(&m_videoCapsStr)).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("appsrc"), StrEq("vidsrc")))
            .WillOnce(Return(GST_ELEMENT(&m_secondaryVideoAppSrc)));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_secondaryVideoAppSrc, &m_videoCaps));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_videoCaps)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void secondarySourceWillBeSetup()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(&m_secondaryRialtoSource));
        // Source will be unreferenced when GstPlayer is destructed.
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_secondaryRialtoSource));
    }

    void willSetupAndAddSecondarySource()
    {
        m_secondaryGstreamerStub.setupAppSrcCallbacks(&m_secondaryVideoAppSrc);
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(&m_secondaryVideoAppSrc), StrEq("block")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(&m_secondaryVideoAppSrc), StrEq("format")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(&m_secondaryVideoAppSrc), StrEq("stream-type")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(&m_secondaryVideoAppSrc), StrEq("min-percent")));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(&m_secondaryVideoAppSrc, (8 * 1024 * 1024)));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetStreamType(&m_secondaryVideoAppSrc, GST_APP_STREAM_TYPE_SEEKABLE));
        EXPECT_CALL(*m_glibWrapperMock, gStrdupPrintfStub(_)).WillOnce(Return(m_sourceName.data())).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock,
                    gstBinAdd(GST_BIN(&m_secondaryRialtoSource), GST_ELEMENT(&m_secondaryVideoAppSrc)));
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(GST_ELEMENT(&m_secondaryVideoAppSrc), StrEq("src")))
            .WillOnce(Return(&m_pad));
        EXPECT_CALL(*m_gstWrapperMock, gstGhostPadNew(StrEq(m_sourceName), &m_pad))
            .WillOnce(Return(&m_ghostPad))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstPadSetQueryFunction(&m_ghostPad, NotNullMatcher())).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstPadSetActive(&m_ghostPad, TRUE)).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstElementAddPad(GST_ELEMENT(&m_secondaryRialtoSource), &m_ghostPad))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pad)).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstElementSyncStateWithParent(GST_ELEMENT(&m_secondaryVideoAppSrc)));
        EXPECT_CALL(*m_glibWrapperMock, gFree(PtrStrMatcher(m_sourceName.data()))).RetiresOnSaturation();
    }

    void willFinishSetupAndAddSecondarySource()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementNoMorePads(GST_ELEMENT(&m_secondaryRialtoSource)));
    }

    void secondaryWillPlay()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_secondaryPipeline, GST_STATE_PLAYING))
            .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
        EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(_, _, _, _))
            .WillRepeatedly(DoAll(SetArgPointee<1>(GST_STATE_NULL), SetArgPointee<2>(GST_STATE_PLAYING),
                                  SetArgPointee<3>(GST_STATE_NULL)));
        EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_NULL))
            .WillRepeatedly(Return(kDummyStateName.c_str()));
        EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_PLAYING))
            .WillRepeatedly(Return(kDummyStateName.c_str()));
        EXPECT_CALL(*m_gstWrapperMock, gstDebugBinToDotFileWithTs(GST_BIN(&m_secondaryPipeline), _, _)).RetiresOnSaturation();
    }

    void willPushSecondaryVideoData(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment, GstBuffer &buffer,
                                    GstCaps &capsCopy, bool shouldNotify)
    {
        std::string dataCopy(segment->getData(), segment->getData() + segment->getDataLength());
        EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, segment->getDataLength(), nullptr))
            .WillOnce(Return(&buffer))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, BufferMatcher(dataCopy), segment->getDataLength()))
            .WillOnce(Return(segment->getDataLength()))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(&m_secondaryVideoAppSrc))
            .WillOnce(Return(&m_videoCaps))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&m_videoCaps)).WillOnce(Return(&capsCopy)).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("width"), G_TYPE_INT, kWidth));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("height"), G_TYPE_INT, kHeight));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstCapsSetSimpleFractionStub(&capsCopy, StrEq("framerate"), GST_TYPE_FRACTION, _, _));
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_secondaryVideoAppSrc, &capsCopy));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_videoCaps)).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&capsCopy));
        if (!shouldNotify)
        {
            EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_secondaryVideoAppSrc, _))
                .InSequence(m_writeBufferSeq)
                .RetiresOnSaturation();
        }
        else
        {
            EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_secondaryVideoAppSrc, _))
                .InSequence(m_writeBufferSeq)
                .WillOnce(Invoke(
                    [this](GstAppSrc *appsrc, GstBuffer *buffer)
                    {
                        workerFinished();
                        return GST_FLOW_OK;
                    }))
                .RetiresOnSaturation();
        }
    }

    void secondaryWillStop()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_secondaryPipeline, GST_STATE_NULL))
            .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
        EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(_, _, _, _))
            .WillRepeatedly(DoAll(SetArgPointee<1>(GST_STATE_NULL), SetArgPointee<2>(GST_STATE_NULL),
                                  SetArgPointee<3>(GST_STATE_NULL)));
        EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_NULL))
            .WillRepeatedly(Return(kDummyStateName.c_str()));
        EXPECT_CALL(*m_gstWrapperMock, gstDebugBinToDotFileWithTs(GST_BIN(&m_secondaryPipeline), _, _)).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_secondaryBus));
    }

    void secondaryGstPlayerWillBeDestructed()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_secondaryPipeline, GST_STATE_NULL))
            .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
        EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_secondaryPipeline)))
            .WillOnce(Return(&m_secondaryBus));
        EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_secondaryBus, nullptr, nullptr, nullptr));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_secondaryBus));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_secondaryPipeline));
    }

    void createSecondarySession()
    {
        // Use matchResponse to store session id
        auto request{createCreateSessionRequest()};
        ConfigureAction<CreateSession>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const ::firebolt::rialto::CreateSessionResponse &resp)
                           { m_secondarySessionId = resp.session_id(); });
    }

    void loadSecondary()
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        auto request = createLoadRequest(m_secondarySessionId);
        ConfigureAction<Load>(m_clientStub).send(request).expectSuccess();

        auto receivedNetworkStateChange = expectedNetworkStateChange.getMessage();
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_secondarySessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(),
                  ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING);
    }

    void attachSecondaryVideoSource()
    {
        auto attachVideoSourceReq{createAttachVideoSourceRequest(m_secondarySessionId)};
        ConfigureAction<AttachSource>(m_clientStub)
            .send(attachVideoSourceReq)
            .expectSuccess()
            .matchResponse([&](const auto &resp) { m_secondaryVideoSourceId = resp.source_id(); });
        waitWorker();
    }

    void setupSecondarySource() { m_secondaryGstreamerStub.setupRialtoSource(); }

    void indicateAllSecondarySourcesAttached()
    {
        ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange(m_clientStub);

        auto allSourcesAttachedReq{createAllSourcesAttachedRequest(m_secondarySessionId)};
        ConfigureAction<AllSourcesAttached>(m_clientStub).send(allSourcesAttachedReq).expectSuccess();

        auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedPlaybackStateChange);
        EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_secondarySessionId);
        EXPECT_EQ(receivedPlaybackStateChange->state(), ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_IDLE);
    }

    void playSecondary()
    {
        auto playReq{createPlayRequest(m_secondarySessionId)};
        ConfigureAction<Play>(m_clientStub).send(playReq).expectSuccess();

        ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};
        expectedPlaybackStateChange.setFilter([&](const auto &event)
                                              { return event.session_id() == m_secondarySessionId; });

        m_secondaryGstreamerStub.sendStateChanged(GST_STATE_NULL, GST_STATE_PLAYING, GST_STATE_NULL);

        auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedPlaybackStateChange);
        EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_secondarySessionId);
        EXPECT_EQ(receivedPlaybackStateChange->state(),
                  ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PLAYING);
    }

    void secondaryGstNeedData()
    {
        ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
        m_secondaryGstreamerStub.needData(&m_secondaryVideoAppSrc, 1);
        auto receivedNeedData{expectedNeedData.getMessage()};
        ASSERT_TRUE(receivedNeedData);
        EXPECT_EQ(receivedNeedData->session_id(), m_secondarySessionId);
        EXPECT_EQ(receivedNeedData->source_id(), m_secondaryVideoSourceId);
        EXPECT_EQ(receivedNeedData->frame_count(), kFrameCountInPlayingState);
        m_lastSecondaryNeedData = receivedNeedData;
    }

    void pushSecondaryVideoData()
    {
        // First, generate new data
        std::unique_ptr<IMediaPipeline::MediaSegment> segment{
            SegmentBuilder().basicVideoSegment(m_secondaryVideoSourceId)()};
        GstBuffer buffer{};
        GstCaps capsCopy{};

        // Next, create frame writer
        ASSERT_TRUE(m_lastSecondaryNeedData);
        std::shared_ptr<MediaPlayerShmInfo> shmInfo{std::make_shared<MediaPlayerShmInfo>(
            MediaPlayerShmInfo{m_lastSecondaryNeedData->shm_info().max_metadata_bytes(),
                               m_lastSecondaryNeedData->shm_info().metadata_offset(),
                               m_lastSecondaryNeedData->shm_info().media_data_offset(),
                               m_lastSecondaryNeedData->shm_info().max_media_bytes()})};
        auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

        // Write frames to shm and add gst expects
        EXPECT_EQ(writer->writeFrame(segment), AddSegmentStatus::OK);
        willPushSecondaryVideoData(segment, buffer, capsCopy, false);

        // Finally, send HaveData and receive new NeedData
        ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
        auto haveDataReq{
            createHaveDataRequest(m_secondarySessionId, writer->getNumFrames(), m_lastSecondaryNeedData->request_id())};
        ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
        auto receivedNeedData{expectedNeedData.getMessage()};
        ASSERT_TRUE(receivedNeedData);
        EXPECT_EQ(receivedNeedData->session_id(), m_secondarySessionId);
        EXPECT_EQ(receivedNeedData->source_id(), m_secondaryVideoSourceId);
        EXPECT_EQ(receivedNeedData->frame_count(), kFrameCountInPlayingState);
        m_lastSecondaryNeedData = receivedNeedData;
    }

    void eosSecondaryVideo()
    {
        // First, generate new data
        std::unique_ptr<IMediaPipeline::MediaSegment> segment{
            SegmentBuilder().basicVideoSegment(m_secondaryVideoSourceId)()};
        GstBuffer buffer{};
        GstCaps capsCopy{};

        // Next, create frame writer
        ASSERT_TRUE(m_lastSecondaryNeedData);
        std::shared_ptr<MediaPlayerShmInfo> shmInfo{std::make_shared<MediaPlayerShmInfo>(
            MediaPlayerShmInfo{m_lastSecondaryNeedData->shm_info().max_metadata_bytes(),
                               m_lastSecondaryNeedData->shm_info().metadata_offset(),
                               m_lastSecondaryNeedData->shm_info().media_data_offset(),
                               m_lastSecondaryNeedData->shm_info().max_media_bytes()})};
        auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

        // Write frames to shm and add gst expects
        EXPECT_EQ(writer->writeFrame(segment), AddSegmentStatus::OK);
        willPushSecondaryVideoData(segment, buffer, capsCopy, true);

        // Finally, send HaveData with EOS status
        auto haveDataReq{
            createHaveDataRequest(m_secondarySessionId, writer->getNumFrames(), m_lastSecondaryNeedData->request_id())};
        haveDataReq.set_status(HaveDataRequest_MediaSourceStatus_EOS);
        ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
        waitWorker();
    }

    void secondaryGstNotifyEos()
    {
        ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};

        m_secondaryGstreamerStub.sendEos();

        auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedPlaybackStateChange);
        EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_secondarySessionId);
        EXPECT_EQ(receivedPlaybackStateChange->state(),
                  ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_END_OF_STREAM);
    }

    void stopSecondary()
    {
        auto stopReq{createStopRequest(m_secondarySessionId)};
        ConfigureAction<Stop>(m_clientStub).send(stopReq).expectSuccess();

        ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};
        expectedPlaybackStateChange.setFilter([&](const auto &event)
                                              { return event.session_id() == m_secondarySessionId; });

        m_secondaryGstreamerStub.sendStateChanged(GST_STATE_NULL, GST_STATE_NULL, GST_STATE_NULL);

        auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
        ASSERT_TRUE(receivedPlaybackStateChange);
        EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_secondarySessionId);
        EXPECT_EQ(receivedPlaybackStateChange->state(),
                  ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_STOPPED);
    }

    void destroySecondarySession()
    {
        auto destroySessionReq{createDestroySessionRequest(m_secondarySessionId)};
        ConfigureAction<DestroySession>(m_clientStub).send(destroySessionReq).expectSuccess();
    }

public:
    int m_secondarySessionId{-1};
    int m_secondaryVideoSourceId{-1};
    GstElement m_secondaryPipeline{};
    GstBus m_secondaryBus{};
    GstElement m_secondaryPlaysink{};
    GstBin m_secondaryRialtoSrcBin = {};
    GstRialtoSrcPrivate m_secondaryRialtoSrcPriv = {};
    GstRialtoSrc m_secondaryRialtoSource = {m_secondaryRialtoSrcBin, &m_secondaryRialtoSrcPriv};
    GstreamerStub m_secondaryGstreamerStub{m_glibWrapperMock, m_gstWrapperMock, &m_secondaryPipeline, &m_secondaryBus,
                                           GST_ELEMENT(&m_secondaryRialtoSource)};
    GstAppSrc m_secondaryVideoAppSrc{};
    std::shared_ptr<::firebolt::rialto::NeedMediaDataEvent> m_lastSecondaryNeedData{nullptr};
};
/*
 * Component Test: Dual Video Playback Sequence
 * Test Objective:
 *  Test the playback of dual video content. Check that all states are transitioned successfully
 *  and data is transferred to gstreamer.
 *
 * Sequence Diagrams:
 *  Create, Destroy - https://wiki.rdkcentral.com/pages/viewpage.action?pageId=226375556
 *  Start/Resume Playback, Pause Playback, Stop, End of stream
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  Map Shared Memory
 *
 * Test Steps:
 *  Step 1: Create a primary media session
 *   Send CreateSessionRequest to Rialto Server
 *   Expect that successful CreateSessionResponse is received
 *   Save returned session id
 *
 *  Step 2: Load content for primary media session
 *   Send LoadRequest to Rialto Server
 *   Expect that successful LoadResponse is received
 *   Expect that GstPlayer instance is created.
 *   Expect that client is notified that the NetworkState has changed to BUFFERING.
 *
 *  Step 3: Attach all sources for primary media session
 *   Attach the audio source.
 *   Expect that audio source is attached.
 *   Attach the video source.
 *   Expect that video source is attached.
 *   Expect that rialto source is setup
 *   Expect that all sources are attached.
 *   Expect that the Playback state has changed to IDLE.
 *
 *  Step 4: Create a secondary media session
 *   Send CreateSessionRequest to Rialto Server
 *   Expect that successful CreateSessionResponse is received
 *   Save returned session id
 *
 *  Step 5: Load content for secondary media session
 *   Send LoadRequest to Rialto Server
 *   Expect that successful LoadResponse is received
 *   Expect that GstPlayer instance is created.
 *   Expect that client is notified that the NetworkState has changed to BUFFERING.
 *
 *  Step 6: Attach video only source to secondary
 *   Attach the video source.
 *   Expect that video source is attached.
 *   Expect that rialto source is setup
 *   Expect that all sources are attached.
 *   Expect that the Playback state has changed to IDLE.
 *
 *  Step 7: Play for primary media session
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 8: Play for secondary media session
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 9: Push initial data for primary session
 *   Gstreamer Stub notifies, that it needs audio data
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Write 1 frame of audio data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Gstreamer Stub notifies, that it needs video data
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Write 1 frame of video data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *
 *  Step 10: Push initial data for secondary session
 *   Gstreamer Stub notifies, that it needs audio data
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Write 1 frame of audio data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Gstreamer Stub notifies, that it needs video data
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Write 1 frame of video data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *
 *  Step 11: End of stream for secondary session
 *   Send audio haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *   Send video haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 12: Notify end of stream for secondary media session
 *   Simulate, that gst_message_eos is received by Rialto Server
 *   Expect that server notifies the client that the Network state has changed to END_OF_STREAM.
 *
 *  Step 13: End of stream for primary session
 *   Send audio haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *   Send video haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 14: Notify end of stream for primary media session
 *   Simulate, that gst_message_eos is received by Rialto Server
 *   Expect that server notifies the client that the Network state has changed to END_OF_STREAM.
 *
 *  Step 15: Terminate the secondary media session
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 *  Step 16: Terminate the primary media session
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *  The state of the Gstreamer Pipeline is successfully negotiationed in the normal playback scenario.
 *  Data is successfully read from the shared memory and pushed to gstreamer pipeline for both audio and video.
 *
 * Code:
 */
TEST_F(DualVideoPlaybackTest, playback)
{
    // Step 1: Create a primary media session
    createSession();

    // Step 2: Load content for primary media session
    gstPlayerWillBeCreated();
    load();

    // Step 3: Attach all sources for primary media session
    audioSourceWillBeAttached();
    attachAudioSource();
    videoSourceWillBeAttached();
    attachVideoSource();
    sourceWillBeSetup();
    setupSource();
    willSetupAndAddSource(&m_audioAppSrc);
    willSetupAndAddSource(&m_videoAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached();

    // Step 4: Create a secondary media session
    createSecondarySession();

    // Step 5: Load content for secondary media session
    secondaryGstPlayerWillBeCreated();
    loadSecondary();

    // Step 6: Attach video only source to secondary
    secondaryVideoSourceWillBeAttached();
    attachSecondaryVideoSource();
    secondarySourceWillBeSetup();
    setupSecondarySource();
    willSetupAndAddSecondarySource();
    willFinishSetupAndAddSecondarySource();
    indicateAllSecondarySourcesAttached();

    // Step 7: Play for primary media session
    willPlay();
    play();

    // Step 8: Play for secondary media session
    secondaryWillPlay();
    playSecondary();

    // Step 9: Push initial data for primary session
    gstNeedData(&m_audioAppSrc, kFrameCountInPlayingState);
    gstNeedData(&m_videoAppSrc, kFrameCountInPlayingState);
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPush, kFrameCountInPlayingState);
        pushVideoData(kFramesToPush, kFrameCountInPlayingState);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }

    // Step 10: Push initial data for secondary session
    secondaryGstNeedData();
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushSecondaryVideoData();

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_secondarySessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }

    // Step 11: End of stream for secondary session
    willEos(&m_secondaryVideoAppSrc);
    eosSecondaryVideo();

    // Step 12: Notify end of stream for secondary media session
    secondaryGstNotifyEos();

    // Step 13: End of stream for primary session
    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPush);
    willEos(&m_videoAppSrc);
    eosVideo(kFramesToPush);

    // Step 14: Notify end of stream for primary media session
    gstNotifyEos();

    // Step 15: Terminate the secondary media session
    secondaryWillStop();
    stopSecondary();
    secondaryGstPlayerWillBeDestructed();
    destroySecondarySession();

    // Step 16: Terminate the primary media session
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);
    willStop();
    stop();
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
