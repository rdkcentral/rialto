/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "GstPlayerTestCommon.h"
#include "Matchers.h"
#include "PlayerTaskMock.h"
#include "TimerMock.h"

using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Return;

namespace
{
constexpr std::chrono::milliseconds positionReportTimerMs{250};
constexpr int32_t kSampleRate{13};
constexpr int32_t kNumberOfChannels{4};
constexpr int32_t kWidth{1024};
constexpr int32_t kHeight{768};
constexpr int32_t kSourceId{2};
constexpr int64_t kTimeStamp{123};
constexpr int64_t kDuration{432};
constexpr int32_t kMediaKeySessionId{4235};
const std::vector<uint8_t> kKeyId{1, 2, 3, 4};
const std::vector<uint8_t> kInitVector{5, 6, 7, 8};
constexpr uint32_t kInitWithLast15{1};
constexpr size_t kNumClearBytes{3};
constexpr size_t kNumEncryptedBytes{5};
} // namespace

struct GstPlayerPrivateTest : public GstPlayerTestCommon
{
    std::unique_ptr<IGstPlayerPrivate> m_sut;

    GstPlayerPrivateTest()
    {
        gstPlayerWillBeCreated();
        m_sut = std::make_unique<GstPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, MediaType::MSE,
                                            m_gstWrapperMock, m_glibWrapperMock, m_gstSrcFactoryMock,
                                            m_timerFactoryMock, std::move(taskFactory), std::move(workerThreadFactory),
                                            std::move(gstDispatcherThreadFactory));
    }

    ~GstPlayerPrivateTest() override
    {
        gstPlayerWillBeDestroyed();
        m_sut.reset();
    }

    void modifyContext(const std::function<void(PlayerContext &)> &fun)
    {
        // Call any method to modify GstPlayer context
        GstAppSrc appSrc{};
        std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
        EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
        EXPECT_CALL(m_taskFactoryMock, createNeedData(_, &appSrc))
            .WillOnce(Invoke([&](PlayerContext &context, GstAppSrc *src) {
                fun(context);
                return std::move(task);
            }));

        m_sut->scheduleNeedMediaData(&appSrc);
    }
};

TEST_F(GstPlayerPrivateTest, shouldScheduleSourceSetupFinish)
{
    std::chrono::milliseconds expectedTimeout{200};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());

    EXPECT_CALL(*m_timerFactoryMock, createTimer(expectedTimeout, _, common::TimerType::ONE_SHOT))
        .WillOnce(Invoke([](const std::chrono::milliseconds &, const std::function<void()> &callback, common::TimerType) {
            callback();
            return std::unique_ptr<common::ITimer>();
        }));
    EXPECT_CALL(m_taskFactoryMock, createFinishSetupSource(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleSourceSetupFinish();
}

TEST_F(GstPlayerPrivateTest, shouldScheduleNeedData)
{
    GstAppSrc appSrc{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createNeedData(_, &appSrc)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleNeedMediaData(&appSrc);
}

TEST_F(GstPlayerPrivateTest, shouldScheduleEnoughDataData)
{
    GstAppSrc appSrc{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createEnoughData(_, &appSrc)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleEnoughData(&appSrc);
}

TEST_F(GstPlayerPrivateTest, shouldScheduleAudioUnderflow)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createUnderflow(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleAudioUnderflow();
}

TEST_F(GstPlayerPrivateTest, shouldScheduleVideoUnderflow)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createUnderflow(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->scheduleVideoUnderflow();
}

TEST_F(GstPlayerPrivateTest, shouldNotSetVideoRectangleWhenVideoSinkIsNull)
{
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, CharStrMatcher("video-sink"), _));
    EXPECT_FALSE(m_sut->setWesterossinkRectangle());
}

TEST_F(GstPlayerPrivateTest, shouldNotSetVideoRectangleWhenVideoSinkDoesNotHaveRectangleProperty)
{
    GstElement videoSink{};
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, CharStrMatcher("video-sink"), _))
        .WillOnce(Invoke([&](gpointer object, const gchar *first_property_name, void *element) {
            GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
            *elementPtr = &videoSink;
        }));
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, CharStrMatcher("rectangle"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&videoSink));
    EXPECT_FALSE(m_sut->setWesterossinkRectangle());
}

TEST_F(GstPlayerPrivateTest, shouldSetVideoRectangle)
{
    GstElement videoSink{};
    GParamSpec rectangleSpec{};
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, CharStrMatcher("video-sink"), _))
        .WillOnce(Invoke([&](gpointer object, const gchar *first_property_name, void *element) {
            GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
            *elementPtr = &videoSink;
        }));
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, CharStrMatcher("rectangle")))
        .WillOnce(Return(&rectangleSpec));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, CharStrMatcher("rectangle")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&videoSink));
    EXPECT_TRUE(m_sut->setWesterossinkRectangle());
}

TEST_F(GstPlayerPrivateTest, shouldNotifyNeedAudioData)
{
    modifyContext([&](PlayerContext &context) { context.audioNeedData = true; });

    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(MediaSourceType::AUDIO)).WillOnce(Return(true));
    m_sut->notifyNeedMediaData(true, false);
}

TEST_F(GstPlayerPrivateTest, shouldNotifyNeedVideoData)
{
    modifyContext([&](PlayerContext &context) { context.videoNeedData = true; });

    EXPECT_CALL(m_gstPlayerClient, notifyNeedMediaData(MediaSourceType::VIDEO)).WillOnce(Return(true));
    m_sut->notifyNeedMediaData(false, true);
}

TEST_F(GstPlayerPrivateTest, shouldNotNotifyNeedAudioDataWhenNotNeeded)
{
    m_sut->notifyNeedMediaData(true, false);
}

TEST_F(GstPlayerPrivateTest, shouldNotNotifyNeedVideoDataWhenNotNeeded)
{
    m_sut->notifyNeedMediaData(false, true);
}

TEST_F(GstPlayerPrivateTest, shouldCreateGstBuffer)
{
    GstBuffer buffer{};
    IMediaPipeline::MediaSegmentVideo mediaSegment{kSourceId, kTimeStamp, kDuration, kWidth, kHeight};
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getDataLength(), nullptr))
        .WillOnce(Return(&buffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, mediaSegment.getData(), mediaSegment.getDataLength()));
    m_sut->createDecryptedBuffer(mediaSegment);
    EXPECT_EQ(GST_BUFFER_TIMESTAMP(&buffer), kTimeStamp);
    EXPECT_EQ(GST_BUFFER_DURATION(&buffer), kDuration);
}

TEST_F(GstPlayerPrivateTest, shouldCreateAndDecryptGstBuffer)
{
    GstBuffer buffer{}, initVectorBuffer{}, keyIdBuffer{}, subSamplesBuffer{};
    guint8 subSamplesData{0};
    auto subSamplesSize{sizeof(guint16) + sizeof(guint32)};
    IMediaPipeline::MediaSegmentVideo mediaSegment{kSourceId, kTimeStamp, kDuration, kWidth, kHeight};
    mediaSegment.setEncrypted(true);
    mediaSegment.setMediaKeySessionId(kMediaKeySessionId);
    mediaSegment.setKeyId(kKeyId);
    mediaSegment.setInitVector(kInitVector);
    mediaSegment.addSubSample(kNumClearBytes, kNumEncryptedBytes);
    mediaSegment.setInitWithLast15(kInitWithLast15);
    testing::InSequence s;
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getDataLength(), nullptr))
        .WillOnce(Return(&buffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, mediaSegment.getData(), mediaSegment.getDataLength()));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getKeyId().size(), nullptr))
        .WillOnce(Return(&keyIdBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&keyIdBuffer, 0, _, mediaSegment.getKeyId().size()));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, mediaSegment.getInitVector().size(), nullptr))
        .WillOnce(Return(&initVectorBuffer));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&initVectorBuffer, 0, _, mediaSegment.getInitVector().size()));
    EXPECT_CALL(*m_glibWrapperMock, gMalloc(subSamplesSize)).WillOnce(Return(&subSamplesData));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterInitWithData(_, &subSamplesData, subSamplesSize, FALSE));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint16Be(_, kNumClearBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstByteWriterPutUint32Be(_, kNumEncryptedBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewWrapped(&subSamplesData, subSamplesSize)).WillOnce(Return(&subSamplesBuffer));
    EXPECT_CALL(m_decryptionServiceMock,
                decrypt(kMediaKeySessionId, &buffer, &subSamplesBuffer, mediaSegment.getSubSamples().size(),
                        &initVectorBuffer, &keyIdBuffer, kInitWithLast15));
    m_sut->createDecryptedBuffer(mediaSegment);
    EXPECT_EQ(GST_BUFFER_TIMESTAMP(&buffer), kTimeStamp);
    EXPECT_EQ(GST_BUFFER_DURATION(&buffer), kDuration);
}

TEST_F(GstPlayerPrivateTest, shouldNotAttachAudioDataWhenBuffersAreEmpty)
{
    m_sut->attachAudioData();
}

TEST_F(GstPlayerPrivateTest, shouldNotAttachAudioDataWhenItIsNotNeeded)
{
    GstBuffer buffer{};
    modifyContext([&](PlayerContext &context) { context.audioBuffers.emplace_back(&buffer); });
    m_sut->attachAudioData();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buffer)); // In destructor
}

TEST_F(GstPlayerPrivateTest, shouldNotAttachAudioWhenSourceIsNotPresent)
{
    GstBuffer buffer{};
    modifyContext([&](PlayerContext &context) {
        context.audioBuffers.emplace_back(&buffer);
        context.audioNeedData = true;
    });
    m_sut->attachAudioData();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buffer)); // In destructor
}

TEST_F(GstPlayerPrivateTest, shouldAttachAudioData)
{
    GstBuffer buffer{};
    GstAppSrc audioSrc{};
    modifyContext([&](PlayerContext &context) {
        context.audioBuffers.emplace_back(&buffer);
        context.audioNeedData = true;
        context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO] = GST_ELEMENT(&audioSrc);
    });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));
    m_sut->attachAudioData();
}

TEST_F(GstPlayerPrivateTest, shouldCancelAudioUnderflow)
{
    GstBuffer buffer{};
    GstAppSrc audioSrc{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](PlayerContext &context) {
        context.audioBuffers.emplace_back(&buffer);
        context.audioNeedData = true;
        context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO] = GST_ELEMENT(&audioSrc);
        context.audioUnderflowOccured = true;
    });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPlay(_)).WillOnce(Return(ByMove(std::move(task))));
    EXPECT_CALL(m_gstPlayerClient, notifyNetworkState(NetworkState::BUFFERED));
    m_sut->attachAudioData();
}

TEST_F(GstPlayerPrivateTest, shouldNotCancelAudioUnderflowWhenVideoUnderflowIsActive)
{
    GstBuffer buffer{};
    GstAppSrc audioSrc{};
    modifyContext([&](PlayerContext &context) {
        context.audioBuffers.emplace_back(&buffer);
        context.audioNeedData = true;
        context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO] = GST_ELEMENT(&audioSrc);
        context.audioUnderflowOccured = true;
        context.videoUnderflowOccured = true;
    });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));
    m_sut->attachAudioData();
}

TEST_F(GstPlayerPrivateTest, shouldNotAttachVideoDataWhenBuffersAreEmpty)
{
    m_sut->attachVideoData();
}

TEST_F(GstPlayerPrivateTest, shouldNotAttachVideoDataWhenItIsNotNeeded)
{
    GstBuffer buffer{};
    modifyContext([&](PlayerContext &context) { context.videoBuffers.emplace_back(&buffer); });
    m_sut->attachVideoData();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buffer)); // In destructor
}

TEST_F(GstPlayerPrivateTest, shouldNotAttachVideoWhenSourceIsNotPresent)
{
    GstBuffer buffer{};
    modifyContext([&](PlayerContext &context) {
        context.videoBuffers.emplace_back(&buffer);
        context.videoNeedData = true;
    });
    m_sut->attachVideoData();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&buffer)); // In destructor
}

TEST_F(GstPlayerPrivateTest, shouldAttachVideoData)
{
    GstBuffer buffer{};
    GstAppSrc videoSrc{};
    modifyContext([&](PlayerContext &context) {
        context.videoBuffers.emplace_back(&buffer);
        context.videoNeedData = true;
        context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO] = GST_ELEMENT(&videoSrc);
    });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));
    m_sut->attachVideoData();
}

TEST_F(GstPlayerPrivateTest, shouldCancelVideoUnderflow)
{
    GstBuffer buffer{};
    GstAppSrc videoSrc{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](PlayerContext &context) {
        context.videoBuffers.emplace_back(&buffer);
        context.videoNeedData = true;
        context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO] = GST_ELEMENT(&videoSrc);
        context.videoUnderflowOccured = true;
    });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPlay(_)).WillOnce(Return(ByMove(std::move(task))));
    EXPECT_CALL(m_gstPlayerClient, notifyNetworkState(NetworkState::BUFFERED));
    m_sut->attachVideoData();
}

TEST_F(GstPlayerPrivateTest, shouldNotCancelVideoUnderflowWhenAudioUnderflowIsActive)
{
    GstBuffer buffer{};
    GstAppSrc videoSrc{};
    modifyContext([&](PlayerContext &context) {
        context.videoBuffers.emplace_back(&buffer);
        context.videoNeedData = true;
        context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO] = GST_ELEMENT(&videoSrc);
        context.audioUnderflowOccured = true;
        context.videoUnderflowOccured = true;
    });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &buffer));
    m_sut->attachVideoData();
}

TEST_F(GstPlayerPrivateTest, shouldAttachAudioAndVideoData)
{
    GstBuffer audioBuffer{};
    GstBuffer videoBuffer{};
    GstAppSrc audioSrc{};
    GstAppSrc videoSrc{};
    modifyContext([&](PlayerContext &context) {
        context.audioBuffers.emplace_back(&audioBuffer);
        context.videoBuffers.emplace_back(&videoBuffer);
        context.audioNeedData = true;
        context.videoNeedData = true;
        context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO] = GST_ELEMENT(&audioSrc);
        context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO] = GST_ELEMENT(&videoSrc);
    });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &audioBuffer));
    m_sut->attachAudioData();
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &videoBuffer));
    EXPECT_CALL(m_gstPlayerClient, notifyNetworkState(NetworkState::BUFFERED));
    m_sut->attachVideoData();
}

TEST_F(GstPlayerPrivateTest, shouldCancelAudioAndVideoUnderflow)
{
    GstBuffer audioBuffer{};
    GstBuffer videoBuffer{};
    GstAppSrc audioSrc{};
    GstAppSrc videoSrc{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](PlayerContext &context) {
        context.bufferedNotificationSent = true;
        context.audioBuffers.emplace_back(&audioBuffer);
        context.videoBuffers.emplace_back(&videoBuffer);
        context.audioNeedData = true;
        context.videoNeedData = true;
        context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO] = GST_ELEMENT(&audioSrc);
        context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO] = GST_ELEMENT(&videoSrc);
        context.audioUnderflowOccured = true;
        context.videoUnderflowOccured = true;
    });
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &audioBuffer));
    m_sut->attachAudioData();
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(_, &videoBuffer));
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPlay(_)).WillOnce(Return(ByMove(std::move(task))));
    EXPECT_CALL(m_gstPlayerClient, notifyNetworkState(NetworkState::BUFFERED));
    m_sut->attachVideoData();
}

TEST_F(GstPlayerPrivateTest, shouldUpdateAudioCaps)
{
    GstAppSrc audioSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](PlayerContext &context) {
        context.streamInfo[firebolt::rialto::MediaSourceType::AUDIO] = GST_ELEMENT(&audioSrc);
    });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&audioSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("rate"), G_TYPE_INT, kSampleRate));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("channels"), G_TYPE_INT, kNumberOfChannels));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&audioSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateAudioCaps(kSampleRate, kNumberOfChannels);
}

TEST_F(GstPlayerPrivateTest, shouldNotUpdateAudioCapsWhenNoSrc)
{
    m_sut->updateAudioCaps(kSampleRate, kNumberOfChannels);
}

TEST_F(GstPlayerPrivateTest, shouldUpdateVideoCaps)
{
    GstAppSrc videoSrc{};
    GstCaps dummyCaps1;
    GstCaps dummyCaps2;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    modifyContext([&](PlayerContext &context) {
        context.streamInfo[firebolt::rialto::MediaSourceType::VIDEO] = GST_ELEMENT(&videoSrc);
    });

    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(GST_APP_SRC(&videoSrc))).WillOnce(Return(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&dummyCaps1)).WillOnce(Return(&dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&dummyCaps2, CharStrMatcher("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(GST_APP_SRC(&videoSrc), &dummyCaps2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps1));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&dummyCaps2));

    m_sut->updateVideoCaps(kWidth, kHeight);
}

TEST_F(GstPlayerPrivateTest, shouldNotUpdateAudioVideoCapsWhenNoSrc)
{
    m_sut->updateVideoCaps(kWidth, kHeight);
}

TEST_F(GstPlayerPrivateTest, shouldFailToChangePlaybackStateWhenPipelineIsNull)
{
    GstElement *pipelineCopy; // to make generic test destructor happy :-)
    modifyContext([&](PlayerContext &context) {
        pipelineCopy = context.pipeline;
        context.pipeline = nullptr;
    });
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_sut->changePipelineState(GST_STATE_PLAYING));
    modifyContext([&](PlayerContext &context) { context.pipeline = pipelineCopy; });
}

TEST_F(GstPlayerPrivateTest, shouldFailToChangePlaybackStateWhenSetStateFails)
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(_, GST_STATE_PLAYING)).WillOnce(Return(GST_STATE_CHANGE_FAILURE));
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(PlaybackState::FAILURE));
    EXPECT_FALSE(m_sut->changePipelineState(GST_STATE_PLAYING));
}

TEST_F(GstPlayerPrivateTest, shouldChangePlaybackState)
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(_, GST_STATE_PLAYING)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_TRUE(m_sut->changePipelineState(GST_STATE_PLAYING));
}

TEST_F(GstPlayerPrivateTest, shouldStartPositionReportingTimer)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    EXPECT_CALL(*m_timerFactoryMock, createTimer(positionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Return(ByMove(std::move(timerMock))));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstPlayerPrivateTest, shouldNotStartPositionReportingTimerWhenItIsActive)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    EXPECT_CALL(dynamic_cast<StrictMock<TimerMock> &>(*timerMock), isActive()).WillOnce(Return(true));
    EXPECT_CALL(*m_timerFactoryMock, createTimer(positionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Return(ByMove(std::move(timerMock))));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstPlayerPrivateTest, shouldScheduleReportPositionWhenPositionReportingTimerIsFired)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    std::unique_ptr<IPlayerTask> task2{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task2), execute());
    EXPECT_CALL(m_taskFactoryMock, createReportPosition(_)).WillOnce(Return(ByMove(std::move(task))));
    EXPECT_CALL(m_taskFactoryMock, createCheckAudioUnderflow(_, _)).WillOnce(Return(ByMove(std::move(task2))));
    EXPECT_CALL(*m_timerFactoryMock, createTimer(positionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Invoke([&](const std::chrono::milliseconds &timeout, const std::function<void()> &callback,
                             common::TimerType timerType) {
            callback();
            return std::move(timerMock);
        }));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstPlayerPrivateTest, shouldStopActivePositionReportingTimer)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    EXPECT_CALL(dynamic_cast<StrictMock<TimerMock> &>(*timerMock), isActive()).WillOnce(Return(true));
    EXPECT_CALL(dynamic_cast<StrictMock<TimerMock> &>(*timerMock), cancel());
    EXPECT_CALL(*m_timerFactoryMock, createTimer(positionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Return(ByMove(std::move(timerMock))));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
    m_sut->stopPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstPlayerPrivateTest, shouldNotStopInactivePositionReportingTimer)
{
    std::unique_ptr<common::ITimer> timerMock = std::make_unique<StrictMock<TimerMock>>();
    EXPECT_CALL(dynamic_cast<StrictMock<TimerMock> &>(*timerMock), isActive()).WillOnce(Return(false));
    EXPECT_CALL(*m_timerFactoryMock, createTimer(positionReportTimerMs, _, common::TimerType::PERIODIC))
        .WillOnce(Return(ByMove(std::move(timerMock))));
    m_sut->startPositionReportingAndCheckAudioUnderflowTimer();
    m_sut->stopPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstPlayerPrivateTest, shouldNotStopInactivePositionReportingTimerWhenThereIsNoTimer)
{
    m_sut->stopPositionReportingAndCheckAudioUnderflowTimer();
}

TEST_F(GstPlayerPrivateTest, shouldStopWorkerThread)
{
    EXPECT_CALL(m_workerThreadMock, stop());
    m_sut->stopWorkerThread();
}
