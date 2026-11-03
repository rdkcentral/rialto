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

#include "ClientComponentTest.h"
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <thread>

namespace firebolt::rialto::client::ct
{
class AudioSwitchingTest : public ClientComponentTest
{
public:
    AudioSwitchingTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~AudioSwitchingTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Audio Switching
 * Test Objective:
 *  Test that media pipeline can switch between audio sources multiple times when paused or playing.
 *
 * Sequence Diagrams:
 *  Audio Stream Switching - https://wiki.rdkcentral.com/display/ASP/Rialto+Dynamic+Audio+Stream+Switching
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Initalise a audio video media session paused and prerolled.
 *
 * Test Steps:
 *  Step 1: Remove audio source
 *   Remove the audio source.
 *   Expect that remove source for audio propagated to the server.
 *
 *  Step 2: Attach mpeg audio source
 *   Attach the new audio source.
 *   Expect that attach source for audio propagated to the server.
 *
 *  Step 3: Write audio frame
 *   Write audio frames.
 *
 *  Step 4: Switch audio source to eac
 *   Switch the audio source.
 *   Expect that attach source with switch source flag for audio propagated to the server.
 *
 *  Step 5: Write audio frame
 *   Write audio frames.
 *
 *  Step 6: Play
 *   Play the content.
 *   Expect that play propagated to the server.
 *   Server notifies the client that the Playback state has changed to PLAYING.
 *   Expect that the state change notification is propagated to the client.
 *
 *  Step 7: Switch audio source to mp4
 *   Switch the audio source.
 *   Expect that attach source with switch source flag for audio propagated to the server.
 *
 *  Step 8: Write audio frame
 *   Write audio frames.
 *
 *  Step 9: Switch audio source to eac
 *   Switch the audio source.
 *   Expect that attach source with switch source flag for audio propagated to the server.
 *
 *  Step 10: Write audio frame
 *   Write audio frames.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  The audio source can be removed and added during ongoing playback (paused/playing).
 *  Minimal disruption of the playback, no state changes.
 *
 * Code:
 */
TEST_F(AudioSwitchingTest, multiSwitching)
{
    // Step 1: Remove audio source
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();

    // Step 2: Attach mpeg audio source
    MediaPipelineTestMethods::shouldAttachAudioSourceMpeg();
    MediaPipelineTestMethods::attachSourceAudioMpeg();

    // Step 3: Write audio frame
    writeAudioFrames();

    // Step 4: Switch audio source to eac
    MediaPipelineTestMethods::shouldSwitchSourceEacs();
    MediaPipelineTestMethods::switchSourceEac();

    // Step 5: Write audio frame
    writeAudioFrames();

    // Step 6: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();

    // Step 7: Switch audio source to mp4
    MediaPipelineTestMethods::shouldSwitchSourceMpeg();
    MediaPipelineTestMethods::switchSourceMpeg();

    // Step 8: Write audio frame
    writeAudioFrames();

    // Step 9: Switch audio source to eac
    MediaPipelineTestMethods::shouldSwitchSourceEacs();
    MediaPipelineTestMethods::switchSourceEac();

    // Step 10: Write audio frame
    writeAudioFrames();
}

TEST_F(AudioSwitchingTest, NeedDataEventDuringAttachDoesNotBlockAttachRpc)
{
    constexpr uint32_t kFramesToWrite{1};
    constexpr int32_t kNewAudioSourceId{3};
    std::promise<void> attachRequestReceived;
    std::promise<void> releaseAttachReply;
    std::promise<bool> attachCompleted;
    std::promise<void> needDataDelivered;
    const auto attachRequestReceivedFuture = attachRequestReceived.get_future();
    const auto releaseAttachReplyFuture = releaseAttachReply.get_future().share();
    auto attachCompletedFuture = attachCompleted.get_future();
    const auto needDataDeliveredFuture = needDataDelivered.get_future();

    EXPECT_CALL(*m_mediaPipelineModuleMock, attachSource(_, _, _, _))
        .WillOnce(Invoke([&](::google::protobuf::RpcController *, const ::firebolt::rialto::AttachSourceRequest *,
                             ::firebolt::rialto::AttachSourceResponse *response, ::google::protobuf::Closure *done)
                         {
                             attachRequestReceived.set_value();
                             releaseAttachReplyFuture.wait();
                             response->set_source_id(kNewAudioSourceId);
                             done->Run();
                         }));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNeedMediaData(_, kFramesToWrite, _, _))
        .WillOnce(InvokeWithoutArgs([&] { needDataDelivered.set_value(); }));

    std::thread attachThread([&]
                             {
                                 std::unique_ptr<IMediaPipeline::MediaSource> source =
                                     std::make_unique<IMediaPipeline::MediaSourceAudio>("audio/mp4");
                                 attachCompleted.set_value(m_mediaPipeline->attachSource(source));
                             });

    const auto attachRequestStatus = attachRequestReceivedFuture.wait_for(std::chrono::seconds(1));
    EXPECT_EQ(attachRequestStatus, std::future_status::ready);
    if (attachRequestStatus == std::future_status::ready)
    {
        sendNotifyNeedDataAudioWithoutWaiting(kFramesToWrite);
        EXPECT_EQ(needDataDeliveredFuture.wait_for(std::chrono::milliseconds(100)), std::future_status::timeout);
        EXPECT_EQ(attachCompletedFuture.wait_for(std::chrono::milliseconds(100)), std::future_status::timeout);
    }

    releaseAttachReply.set_value();

    const auto attachStatus = attachCompletedFuture.wait_for(std::chrono::seconds(1));
    EXPECT_EQ(attachStatus, std::future_status::ready);
    if (attachStatus == std::future_status::ready)
    {
        EXPECT_TRUE(attachCompletedFuture.get());
    }
    EXPECT_EQ(needDataDeliveredFuture.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    attachThread.join();
}
} // namespace firebolt::rialto::client::ct
