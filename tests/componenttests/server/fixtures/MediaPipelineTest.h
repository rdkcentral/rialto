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

#ifndef FIREBOLT_RIALTO_SERVER_CT_MEDIA_PIPELINE_TEST_H_
#define FIREBOLT_RIALTO_SERVER_CT_MEDIA_PIPELINE_TEST_H_

#include "GstSrc.h"
#include "GstreamerStub.h"
#include "IMediaPipeline.h"
#include "RialtoServerComponentTest.h"
#include "ShmHandle.h"
#include "mediapipelinemodule.pb.h"
#include <memory>
#include <string>

namespace firebolt::rialto::server::ct
{
class MediaPipelineTest : public RialtoServerComponentTest
{
public:
    MediaPipelineTest();
    ~MediaPipelineTest() override = default;

    void gstPlayerWillBeCreated();
    void gstPlayerWillBeDestructed();
    void audioSourceWillBeAttached();
    void videoSourceWillBeAttached();
    void sourceWillBeSetup();
    void willSetupAndAddSource(GstAppSrc *appSrc);
    void willFinishSetupAndAddSource();
    void willPushAudioData(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment, GstBuffer &buffer,
                           GstCaps &capsCopy, bool shouldNotify);
    void willPushVideoData(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment, GstBuffer &buffer,
                           GstCaps &capsCopy, bool shouldNotify);
    void willPause();
    void willNotifyPaused();
    void willPlay();
    void willEos(GstAppSrc *appSrc);
    void willRemoveAudioSource();
    void willStop();
    void willSetAudioAndVideoFlags();

    void createSession();
    void load();
    void attachAudioSource();
    void attachVideoSource();
    void setupSource();
    void indicateAllSourcesAttached();
    void pause();
    void notifyPaused();
    void gstNeedData(GstAppSrc *appSrc, int frameCount);
    void pushAudioData(unsigned dataCountToPush, int needDataFrameCount);
    void pushVideoData(unsigned dataCountToPush, int needDataFrameCount);
    void play();
    void eosAudio(unsigned dataCountToPush);
    void eosVideo(unsigned dataCountToPush);
    void gstNotifyEos();
    void removeSource(int sourceId);
    void stop();
    void destroySession();
    void waitWorker();
    void workerFinished();

private:
    void initShm();

protected:
    int m_sessionId{-1};
    int m_audioSourceId{-1};
    int m_videoSourceId{-1};
    ShmHandle m_shmHandle;
    GstElement m_pipeline{};
    GstBus m_bus{};
    GstElement m_playsink{};
    GstBin m_rialtoSrcBin = {};
    GstRialtoSrcPrivate m_rialtoSrcPriv = {};
    GstRialtoSrc m_rialtoSource = {m_rialtoSrcBin, &m_rialtoSrcPriv};
    GstreamerStub m_gstreamerStub{m_glibWrapperMock, m_gstWrapperMock, &m_pipeline, &m_bus, GST_ELEMENT(&m_rialtoSource)};
    GstAppSrc m_audioAppSrc{};
    GstAppSrc m_videoAppSrc{};
    GFlagsClass m_flagsClass{};
    GFlagsValue m_audioFlag{1, "audio", "audio"};
    GFlagsValue m_videoFlag{2, "video", "video"};
    GFlagsValue m_nativeVideoFlag{3, "native-video", "native-video"};
    GstCaps m_audioCaps{};
    GstCaps m_videoCaps{};
    gchar m_audioCapsStr{};
    gchar m_videoCapsStr{};
    std::string m_sourceName{"src_0"};
    GstPad m_pad{};
    GstPad m_ghostPad{};
    GstEvent m_flushStartEvent{};
    GstEvent m_flushStopEvent{};
    std::shared_ptr<::firebolt::rialto::NeedMediaDataEvent> m_lastAudioNeedData{nullptr};
    std::shared_ptr<::firebolt::rialto::NeedMediaDataEvent> m_lastVideoNeedData{nullptr};

    // Used to syncronise the writing of the data to gstreamer
    testing::Sequence m_writeBufferSeq;

    // Required to syncronise gstBufferNewAllocate, this method only takes a size parameter, if the
    // size of two buffers are equal the test can fail.
    testing::Sequence m_bufferAllocateSeq;

    // Mock objects should not be used on different threads at the same time, this can lead
    // to race conditions. This mutex and cond is required to avoid the gst worker thread
    // checking expect calls while the main thread is setting expect calls.
    std::mutex m_workerLock;
    std::condition_variable m_workerCond;
    bool m_workerDone{false};
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_MEDIA_PIPELINE_TEST_H_
