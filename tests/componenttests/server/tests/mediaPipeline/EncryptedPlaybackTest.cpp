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
#include "ExpectMessage.h"
#include "IMediaFrameWriter.h"
#include "Matchers.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"
#include "SegmentBuilder.h"

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCountInPausedState{3};
constexpr int kFrameCountInPlayingState{24};
} // namespace

using testing::_;
using testing::Return;

namespace firebolt::rialto::server::ct
{
class EncryptedPlaybackTest : public MediaPipelineTest
{
public:
    EncryptedPlaybackTest() = default;
    ~EncryptedPlaybackTest() override = default;

    void willAddProtectionMetadata(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment, GstBuffer &dataBuffer,
                                   GstBuffer &keyIdBuffer, GstBuffer &initVectorBuffer, GstBuffer &subsamplesBuffer,
                                   guint8 &subsamples, GstMeta &meta)
    {
        const std::size_t kSubsamplesSize{segment->getSubSamples().size() * (sizeof(guint16) + sizeof(guint32))};
        EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, segment->getKeyId().size(), nullptr))
            .InSequence(m_bufferAllocateSeq)
            .WillOnce(Return(&keyIdBuffer))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&keyIdBuffer, 0, _, segment->getKeyId().size()))
            .WillOnce(Return(segment->getKeyId().size()))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, segment->getInitVector().size(), nullptr))
            .InSequence(m_bufferAllocateSeq)
            .WillOnce(Return(&initVectorBuffer))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&initVectorBuffer, 0, _, segment->getInitVector().size()))
            .WillOnce(Return(segment->getInitVector().size()))
            .RetiresOnSaturation();
        EXPECT_CALL(*m_glibWrapperMock, gMalloc(kSubsamplesSize)).WillOnce(Return(&subsamples)).RetiresOnSaturation();
        EXPECT_CALL(*m_gstWrapperMock, gstByteWriterInitWithData(_, &subsamples, kSubsamplesSize, FALSE));
        EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint16Be(_, segment->getSubSamples().front().numClearBytes));
        EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint32Be(_, segment->getSubSamples().front().numEncryptedBytes));
        EXPECT_CALL(*m_gstWrapperMock, gstBufferNewWrapped(&subsamples, kSubsamplesSize)).WillOnce(Return(&subsamplesBuffer));
        EXPECT_CALL(*m_gstWrapperMock, gstBufferAddMeta(&dataBuffer, _, _)).WillOnce(Return(&meta));
    }

    void pushEncryptedAudioData(int needDataFrameCount)
    {
        // First, generate new data
        auto segment{SegmentBuilder().basicAudioSegment(m_audioSourceId).withEncryptionData()()};
        GstBuffer buffer{};
        GstBuffer keyIdBuffer{};
        GstBuffer initVectorBuffer{};
        GstBuffer subsamplesBuffer{};
        guint8 subsamples{};
        GstMeta meta{};
        GstCaps capsCopy{};

        // Next, create frame writer
        ASSERT_TRUE(m_lastAudioNeedData);
        std::shared_ptr<MediaPlayerShmInfo> shmInfo{
            std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{m_lastAudioNeedData->shm_info().max_metadata_bytes(),
                                                                    m_lastAudioNeedData->shm_info().metadata_offset(),
                                                                    m_lastAudioNeedData->shm_info().media_data_offset(),
                                                                    m_lastAudioNeedData->shm_info().max_media_bytes()})};
        auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

        // Write frame to shm and add gst expects
        EXPECT_EQ(writer->writeFrame(segment), AddSegmentStatus::OK);
        willPushAudioData(segment, buffer, capsCopy, false);
        willAddProtectionMetadata(segment, buffer, keyIdBuffer, initVectorBuffer, subsamplesBuffer, subsamples, meta);

        // Finally, send HaveData and receive new NeedData
        ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
        auto haveDataReq{createHaveDataRequest(m_sessionId, writer->getNumFrames(), m_lastAudioNeedData->request_id())};
        ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
        auto receivedNeedData{expectedNeedData.getMessage()};
        ASSERT_TRUE(receivedNeedData);
        EXPECT_EQ(receivedNeedData->session_id(), m_sessionId);
        EXPECT_EQ(receivedNeedData->source_id(), m_audioSourceId);
        EXPECT_EQ(receivedNeedData->frame_count(), needDataFrameCount);
        m_lastAudioNeedData = receivedNeedData;
    }

    void pushEncryptedVideoData(int needDataFrameCount)
    {
        // First, generate new data
        auto segment{SegmentBuilder().basicVideoSegment(m_videoSourceId).withEncryptionData()()};
        GstBuffer buffer{};
        GstBuffer keyIdBuffer{};
        GstBuffer initVectorBuffer{};
        GstBuffer subsamplesBuffer{};
        guint8 subsamples{};
        GstMeta meta{};
        GstCaps capsCopy{};

        // Next, create frame writer
        ASSERT_TRUE(m_lastVideoNeedData);
        std::shared_ptr<MediaPlayerShmInfo> shmInfo{
            std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{m_lastVideoNeedData->shm_info().max_metadata_bytes(),
                                                                    m_lastVideoNeedData->shm_info().metadata_offset(),
                                                                    m_lastVideoNeedData->shm_info().media_data_offset(),
                                                                    m_lastVideoNeedData->shm_info().max_media_bytes()})};
        auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

        // Write frame to shm and add gst expects
        EXPECT_EQ(writer->writeFrame(segment), AddSegmentStatus::OK);
        willPushVideoData(segment, buffer, capsCopy, false);
        willAddProtectionMetadata(segment, buffer, keyIdBuffer, initVectorBuffer, subsamplesBuffer, subsamples, meta);

        // Finally, send HaveData and receive new NeedData
        ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
        auto haveDataReq{createHaveDataRequest(m_sessionId, writer->getNumFrames(), m_lastVideoNeedData->request_id())};
        ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
        auto receivedNeedData{expectedNeedData.getMessage()};
        ASSERT_TRUE(receivedNeedData);
        EXPECT_EQ(receivedNeedData->session_id(), m_sessionId);
        EXPECT_EQ(receivedNeedData->source_id(), m_videoSourceId);
        EXPECT_EQ(receivedNeedData->frame_count(), needDataFrameCount);
        m_lastVideoNeedData = receivedNeedData;
    }
};
/*
 * Component Test: Encrypted Audio Video Playback Sequence
 * Test Objective:
 *  Test the playback of encrypted video and audio MSE content. The test transitions through the playback states
 *  buffering 1 frame of both encrypted audio and video content before preroll and 1 frame of both encrypted audio and
 *  video content after preroll. The session is then terminated. All the metadata and media data written to the
 *  shared buffer is checked for accuracy.
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
 *  Step 1: Create a new media session
 *   Send CreateSessionRequest to Rialto Server
 *   Expect that successful CreateSessionResponse is received
 *   Save returned session id
 *
 *  Step 2: Load content
 *   Send LoadRequest to Rialto Server
 *   Expect that successful LoadResponse is received
 *   Expect that GstPlayer instance is created.
 *   Expect that client is notified that the NetworkState has changed to BUFFERING.
 *
 *  Step 3: Attach all sources
 *   Attach the audio source.
 *   Expect that audio source is attached.
 *   Attach the video source.
 *   Expect that video source is attached.
 *   Expect that rialto source is setup
 *   Expect that all sources are attached.
 *   Expect that the Playback state has changed to IDLE.
 *
 *  Step 4: Pause
 *   Pause the content.
 *   Expect that gstreamer pipeline is paused.
 *
 *  Step 5: Write 1 encrypted audio frame
 *   Gstreamer Stub notifies, that it needs audio data
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Write 1 frame of encrypted audio data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *
 *  Step 6: Write 1 encrypted video frame
 *   Gstreamer Stub notifies, that it needs video data
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Write 1 encrypted frame of video data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *
 *  Step 7: Notify buffered
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *
 *  Step 8: Notify Paused
 *   Gstreamer Stub notifies, that pipeline state is in PAUSED state
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 9: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 10: Write 1 encrypted audio frame
 *   Write 1 encrypted frame of audio data to the shared buffer.
 *   Send HaveData
 *   Expect that server notifies the client that it needs 24 frames of audio data.
 *
 *  Step 11: Write 1 encrypted video frame
 *   Write 1 encrypted frame of video data to the shared buffer.
 *   Send HaveData
 *   Expect that server notifies the client that it needs 24 frames of video data.
 *
 *  Step 12: End of audio stream
 *   Send audio haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 13: End of video stream
 *   Send video haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 14: Notify end of stream
 *   Simulate, that gst_message_eos is received by Rialto Server
 *   Expect that server notifies the client that the Network state has changed to END_OF_STREAM.
 *
 *  Step 15: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 16: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 17: Destroy media session
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
 *  Data is successfully read from the shared memory and pushed to gstreamer pipeline for both audio and video
 *  with protection metadata.
 *
 * Code:
 */
TEST_F(EncryptedPlaybackTest, EncryptedPlayback)
{
    // Step 1: Create a new media session
    createSession();

    // Step 2: Load content
    gstPlayerWillBeCreated();
    load();

    // Step 3: Attach all sources
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

    // Step 4: Pause
    willPause();
    pause();

    // Step 5: Write 1 encrypted audio frame
    // Step 6: Write 1 encrypted video frame
    // Step 7: Notify buffered
    gstNeedData(&m_audioAppSrc, kFrameCountInPausedState);
    gstNeedData(&m_videoAppSrc, kFrameCountInPausedState);
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushEncryptedAudioData(kFrameCountInPausedState);
        pushEncryptedVideoData(kFrameCountInPausedState);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }

    // Step 8: Notify Paused
    willNotifyPaused();
    notifyPaused();

    // Step 9: Play
    willPlay();
    play();

    // Step 10: Write 1 encrypted audio frame
    // Step 11: Write 1 encrypted video frame
    pushEncryptedAudioData(kFrameCountInPlayingState);
    pushEncryptedVideoData(kFrameCountInPlayingState);

    // Step 12: End of audio stream
    // Step 13: End of video stream
    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPush);
    willEos(&m_videoAppSrc);
    eosVideo(kFramesToPush);

    // Step 14: Notify end of stream
    gstNotifyEos();
    willRemoveAudioSource();

    // Step 15: Remove sources
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 16: Stop
    willStop();
    stop();

    // Step 17: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
