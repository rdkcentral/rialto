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

#include "DataReaderMock.h"
#include "GstGenericPlayerTestCommon.h"
#include "HeartbeatHandlerMock.h"
#include "IGstGenericPlayerPrivate.h"
#include "IMediaPipeline.h"
#include "MatchersGenericPlayer.h"
#include "MediaSourceUtil.h"
#include "PlayerTaskMock.h"
#include "TimerMock.h"

using testing::_;
using testing::ByMove;
using testing::DoAll;
using testing::Invoke;
using testing::Ref;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrEq;

class GstGenericPlayerTest : public GstGenericPlayerTestCommon
{
protected:
    std::unique_ptr<IGstGenericPlayer> m_sut;
    VideoRequirements m_videoReq = {kMinPrimaryVideoWidth, kMinPrimaryVideoHeight};
    GstElement *m_pipeline;
    GstIterator m_it{};
    char m_dummy{0};
    GstElementFactory *m_factory = reinterpret_cast<GstElementFactory *>(&m_dummy);
    GstElement *m_element;
    GParamSpec m_prop{};

    GstGenericPlayerTest()
    {
        gstPlayerWillBeCreated();
        m_sut = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, MediaType::MSE,
                                                   m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                   m_rdkGstreamerUtilsWrapperMock, m_gstInitialiserMock,
                                                   std::move(m_flushWatcher), m_gstSrcFactoryMock, m_timerFactoryMock,
                                                   std::move(m_taskFactory), std::move(workerThreadFactory),
                                                   std::move(gstDispatcherThreadFactory),
                                                   m_gstProtectionMetadataFactoryMock);
        m_element = fakeElement();
    }

    ~GstGenericPlayerTest() override
    {
        gst_object_unref(m_element);
        gstPlayerWillBeDestroyed();
        m_sut.reset();
    }

    void getContext(const std::function<void(GenericPlayerContext &)> &fun)
    {
        // Call any method to modify GstGenericPlayer context
        double targetVolume{};
        uint32_t volumeDuration{};
        firebolt::rialto::EaseType easeType{};
        std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
        EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
        EXPECT_CALL(m_taskFactoryMock, createSetVolume(_, _, targetVolume, volumeDuration, easeType))
            .WillOnce(Invoke(
                [&](GenericPlayerContext &m_context, IGstGenericPlayerPrivate &m_player, double targetVolume,
                    uint32_t volumeDuration, firebolt::rialto::EaseType easeType)
                {
                    fun(m_context);
                    return std::move(task);
                }));
        m_sut->setVolume(targetVolume, volumeDuration, easeType);
    }

    GstElement *fakeElement()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        GstElement *elem = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
        return elem;
    }

    template <typename T> void willGetElementProperty(const std::string &propertyName, const T &value)
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq(propertyName.c_str()))).WillOnce(Return(&m_prop));

        if constexpr (std::is_same_v<T, bool>)
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(propertyName.c_str()), _))
                .WillOnce(Invoke(
                    [&](gpointer object, const gchar *first_property_name, void *val)
                    {
                        gboolean *returnVal = reinterpret_cast<gboolean *>(val);
                        *returnVal = value ? TRUE : FALSE;
                    }));
        }
        else if constexpr (std::is_same_v<T, int32_t>)
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(propertyName.c_str()), _))
                .WillOnce(Invoke(
                    [&](gpointer object, const gchar *first_property_name, void *val)
                    {
                        gint *returnVal = reinterpret_cast<gint *>(val);
                        *returnVal = value;
                    }));
        }
        else if constexpr (std::is_same_v<T, uint32_t>)
        {
            EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(propertyName.c_str()), _))
                .WillOnce(Invoke(
                    [&](gpointer object, const gchar *first_property_name, void *val)
                    {
                        guint *returnVal = reinterpret_cast<guint *>(val);
                        *returnVal = value;
                    }));
        }

        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    }
};

TEST_F(GstGenericPlayerTest, shouldAttachSource)
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/mpeg");

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createAttachSource(_, _, Ref(source))).WillOnce(Return(ByMove(std::move(task))));

    m_sut->attachSource(source);
}

TEST_F(GstGenericPlayerTest, shouldRemoveSource)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createRemoveSource(_, _, MediaSourceType::AUDIO))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->removeSource(MediaSourceType::AUDIO);
}

TEST_F(GstGenericPlayerTest, shouldAllSourcesAttached)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createFinishSetupSource(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->allSourcesAttached();
}

// TEST_F(GstGenericPlayerTest, shouldPlay)
// {
//     std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
//     EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
//     EXPECT_CALL(m_taskFactoryMock, createPlay(_)).WillOnce(Return(ByMove(std::move(task))));

//     m_sut->play();
// }

TEST_F(GstGenericPlayerTest, shouldPause)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPause(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->pause();
}

TEST_F(GstGenericPlayerTest, shouldStop)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createStop(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->stop();
}

TEST_F(GstGenericPlayerTest, shouldAttachSamplesFromVector)
{
    IMediaPipeline::MediaSegmentVector mediaSegments;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createAttachSamples(_, _, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->attachSamples(mediaSegments);
}

TEST_F(GstGenericPlayerTest, shouldAttachSamplesFromShm)
{
    std::shared_ptr<IDataReader> dataReader{std::make_shared<DataReaderMock>()};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createReadShmDataAndAttachSamples(_, _, dataReader))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->attachSamples(dataReader);
}

TEST_F(GstGenericPlayerTest, shouldSetPlaybackRate)
{
    double playbackRate{1.5};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetPlaybackRate(_, playbackRate)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setPlaybackRate(playbackRate);
}

TEST_F(GstGenericPlayerTest, shouldSetPosition)
{
    std::int64_t position{12345};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetPosition(_, _, position)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setPosition(position);
}

TEST_F(GstGenericPlayerTest, shouldSetVideoGeometry)
{
    Rectangle rectangle{1, 2, 3, 4};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetVideoGeometry(_, _, rectangle)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setVideoGeometry(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
}

TEST_F(GstGenericPlayerTest, shouldSetEos)
{
    MediaSourceType type{MediaSourceType::AUDIO};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createEos(_, _, type)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setEos(type);
}

TEST_F(GstGenericPlayerTest, shouldSetupSource)
{
    GstElement source{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(&source));
    EXPECT_CALL(m_taskFactoryMock, createSetupSource(_, _, &source)).WillOnce(Return(ByMove(std::move(task))));

    triggerSetupSource(&source);
}

TEST_F(GstGenericPlayerTest, shouldSetupElement)
{
    GstElement element{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(&element));
    EXPECT_CALL(m_taskFactoryMock, createSetupElement(_, _, &element)).WillOnce(Return(ByMove(std::move(task))));

    triggerSetupElement(&element);
}

TEST_F(GstGenericPlayerTest, shouldAddDeepElement)
{
    GstElement element{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createDeepElementAdded(_, _, _, _, &element)).WillOnce(Return(ByMove(std::move(task))));

    triggerDeepElementAdded(&element);
}

TEST_F(GstGenericPlayerTest, shouldReturnInvalidPositionWhenPipelineIsNull)
{
    GstElement *pipeline{};
    getContext(
        [&](GenericPlayerContext &m_context)
        {
            pipeline = m_context.pipeline;
            m_context.pipeline = nullptr;
        });
    int64_t targetPosition{};
    EXPECT_FALSE(m_sut->getPosition(targetPosition));

    // Clean up
    getContext([&](GenericPlayerContext &m_context) { m_context.pipeline = pipeline; });
}

TEST_F(GstGenericPlayerTest, shouldReturnInvalidPositionWhenPipelineIsPrerolling)
{
    int64_t targetPosition{};
    EXPECT_CALL(*m_gstWrapperMock, gstStateLock(_)).WillOnce(Return());
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(_)).WillRepeatedly(Return(GST_STATE_PAUSED));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStateNext(_)).WillRepeatedly(Return(GST_STATE_PAUSED));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStateReturn(_)).WillRepeatedly(Return(GST_STATE_CHANGE_ASYNC));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(_)).WillRepeatedly(Return("NotImporant"));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateChangeReturnGetName(_)).WillRepeatedly(Return("NotImporant"));
    EXPECT_CALL(*m_gstWrapperMock, gstStateUnlock(_)).WillOnce(Return());
    EXPECT_FALSE(m_sut->getPosition(targetPosition));
}

TEST_F(GstGenericPlayerTest, shouldReturnInvalidPositionWhenQueryFails)
{
    int64_t targetPosition{};
    EXPECT_CALL(*m_gstWrapperMock, gstStateLock(_)).WillOnce(Return());
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(_)).WillOnce(Return(GST_STATE_PLAYING));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStateReturn(_)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstStateUnlock(_)).WillOnce(Return());
    EXPECT_CALL(*m_gstWrapperMock, gstElementQueryPosition(_, GST_FORMAT_TIME, _)).WillOnce(Return(FALSE));
    EXPECT_FALSE(m_sut->getPosition(targetPosition));
}

TEST_F(GstGenericPlayerTest, shouldReturnPositionInPlayingState)
{
    constexpr gint64 kExpectedPosition{123};
    int64_t targetPosition{};
    EXPECT_CALL(*m_gstWrapperMock, gstStateLock(_)).WillOnce(Return());
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(_)).WillOnce(Return(GST_STATE_PLAYING));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStateReturn(_)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstStateUnlock(_)).WillOnce(Return());
    EXPECT_CALL(*m_gstWrapperMock, gstElementQueryPosition(_, GST_FORMAT_TIME, _))
        .WillOnce(Invoke(
            [&](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = kExpectedPosition;
                return TRUE;
            }));
    EXPECT_TRUE(m_sut->getPosition(targetPosition));
    EXPECT_EQ(kExpectedPosition, targetPosition);
}

TEST_F(GstGenericPlayerTest, shouldReturnPositionInPausedState)
{
    constexpr gint64 kExpectedPosition{123};
    int64_t targetPosition{};
    EXPECT_CALL(*m_gstWrapperMock, gstStateLock(_)).WillOnce(Return());
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(_)).WillOnce(Return(GST_STATE_PAUSED));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStateReturn(_)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstStateUnlock(_)).WillOnce(Return());
    EXPECT_CALL(*m_gstWrapperMock, gstElementQueryPosition(_, GST_FORMAT_TIME, _))
        .WillOnce(Invoke(
            [&](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = kExpectedPosition;
                return TRUE;
            }));
    EXPECT_TRUE(m_sut->getPosition(targetPosition));
    EXPECT_EQ(kExpectedPosition, targetPosition);
}

TEST_F(GstGenericPlayerTest, shouldSetImmediateOutput)
{
    // There is no need to create a sink in playing state because the task code
    // (that would use the sink) is tested elsewhere.
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetImmediateOutput(_, _, MediaSourceType::VIDEO, true))
        .WillOnce(Return(ByMove(std::move(task))));

    EXPECT_TRUE(m_sut->setImmediateOutput(MediaSourceType::VIDEO, true));
}

TEST_F(GstGenericPlayerTest, shouldGetImmediateOutputInPlayingState)
{
    setPipelineState(GST_STATE_PLAYING);
    const bool kTestImmediateOutputValue{true};
    const std::string kPropertyStr{"immediate-output"};

    expectGetSink(kVideoSinkStr, m_element);
    willGetElementProperty(kPropertyStr, kTestImmediateOutputValue);

    bool immediateOutputState;
    EXPECT_TRUE(m_sut->getImmediateOutput(MediaSourceType::VIDEO, immediateOutputState));
    EXPECT_EQ(immediateOutputState, kTestImmediateOutputValue);
}

TEST_F(GstGenericPlayerTest, shouldGetImmediateOutputInPlayingStateForAudio)
{
    setPipelineState(GST_STATE_PLAYING);
    const bool kTestImmediateOutputValue{true};
    const std::string kPropertyStr{"immediate-output"};

    expectGetSink(kAudioSinkStr, m_element);
    willGetElementProperty(kPropertyStr, kTestImmediateOutputValue);

    bool immediateOutputState;
    EXPECT_TRUE(m_sut->getImmediateOutput(MediaSourceType::AUDIO, immediateOutputState));
    EXPECT_EQ(immediateOutputState, kTestImmediateOutputValue);
}

TEST_F(GstGenericPlayerTest, shouldFailToGetImmediateOutputInPlayingStateIfMediaTypeWrong)
{
    setPipelineState(GST_STATE_PLAYING);

    bool immediateOutputState;
    EXPECT_FALSE(m_sut->getImmediateOutput(MediaSourceType::UNKNOWN, immediateOutputState));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetImmediateOutputInPlayingStateIfStubNull)
{
    setPipelineState(GST_STATE_PLAYING);

    // Fail to get sink which should cause the getImmediateOutput() call to return false
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(kAudioSinkStr), _)).Times(1);

    bool immediateOutputState;
    EXPECT_FALSE(m_sut->getImmediateOutput(MediaSourceType::AUDIO, immediateOutputState));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetImmediateOutputInPlayingStateIfPropertyDoesntExist)
{
    setPipelineState(GST_STATE_PLAYING);

    expectGetSink(kVideoSinkStr, m_element);

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("immediate-output"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);

    bool immediateOutputState;
    EXPECT_FALSE(m_sut->getImmediateOutput(MediaSourceType::VIDEO, immediateOutputState));
}

TEST_F(GstGenericPlayerTest, shouldGetStatsInPlayingState)
{
    constexpr guint64 kRenderedFrames{1234};
    constexpr guint64 kDroppedFrames{5};
    uint64_t returnedRenderedFrames{};
    uint64_t returnedDroppedFrames{};
    setPipelineState(GST_STATE_PLAYING);

    expectGetSink(kVideoSinkStr, m_element);

    GstStructure testStructure;
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstStructure **elementPtr = reinterpret_cast<GstStructure **>(element);
                *elementPtr = &testStructure;
            }));

    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&testStructure, StrEq("rendered"), _))
        .WillOnce(DoAll(SetArgumentPointee<2>(kRenderedFrames), Return(true)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&testStructure, StrEq("dropped"), _))
        .WillOnce(DoAll(SetArgumentPointee<2>(kDroppedFrames), Return(true)));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstStructureFree(&testStructure)).Times(1);

    EXPECT_TRUE(m_sut->getStats(MediaSourceType::VIDEO, returnedRenderedFrames, returnedDroppedFrames));
    EXPECT_EQ(kRenderedFrames, returnedRenderedFrames);
    EXPECT_EQ(kDroppedFrames, returnedDroppedFrames);
}

TEST_F(GstGenericPlayerTest, shouldFailToGetStatsInPlayingStateIfMediaTypeWrong)
{
    setPipelineState(GST_STATE_PLAYING);

    uint64_t returnedRenderedFrames;
    uint64_t returnedDroppedFrames;
    EXPECT_FALSE(m_sut->getStats(MediaSourceType::UNKNOWN, returnedRenderedFrames, returnedDroppedFrames));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetStatsInPlayingStateIfStubNull)
{
    setPipelineState(GST_STATE_PLAYING);

    // Fail to get sink which should cause the getStats() call to return false
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(kAudioSinkStr), _)).Times(1);

    uint64_t returnedRenderedFrames;
    uint64_t returnedDroppedFrames;
    EXPECT_FALSE(m_sut->getStats(MediaSourceType::AUDIO, returnedRenderedFrames, returnedDroppedFrames));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetStatsInPlayingStateIfStructureNull)
{
    setPipelineState(GST_STATE_PLAYING);

    expectGetSink(kAudioSinkStr, m_element);

    // Fail to get GstStructure which should cause the getStats() call to return false
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);

    uint64_t returnedRenderedFrames;
    uint64_t returnedDroppedFrames;
    EXPECT_FALSE(m_sut->getStats(MediaSourceType::AUDIO, returnedRenderedFrames, returnedDroppedFrames));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetStatsInPlayingStateIfStructIncomplete)
{
    setPipelineState(GST_STATE_PLAYING);

    expectGetSink(kVideoSinkStr, m_element);

    GstStructure testStructure;
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstStructure **elementPtr = reinterpret_cast<GstStructure **>(element);
                *elementPtr = &testStructure;
            }));

    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&testStructure, StrEq("rendered"), _)).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstStructureFree(&testStructure)).Times(1);

    uint64_t returnedRenderedFrames;
    uint64_t returnedDroppedFrames;
    EXPECT_FALSE(m_sut->getStats(MediaSourceType::VIDEO, returnedRenderedFrames, returnedDroppedFrames));
}

TEST_F(GstGenericPlayerTest, ShouldGetVolumeWhenAudioSinkIsNull)
{
    setPipelineState(GST_STATE_PLAYING);

    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(kAudioSinkStr), _)).Times(1);

    constexpr double kVolume{0.5};
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeGetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR)).WillOnce(Return(kVolume));

    double currentVolume;
    EXPECT_TRUE(m_sut->getVolume(currentVolume));
}

TEST_F(GstGenericPlayerTest, shouldGetVolumeWithNegativeFadeVolume)
{
    getContext([&](GenericPlayerContext &m_context) { m_context.audioFadeEnabled = true; });
    setPipelineState(GST_STATE_PLAYING);
    const gint kNegativeFadeVolume{-100};
    const std::string kPropertyStr{"fade-volume"};

    expectGetSink(kAudioSinkStr, m_element);
    willGetElementProperty(kPropertyStr, kNegativeFadeVolume);

    constexpr double kVolume{0.5};
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeGetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR)).WillOnce(Return(kVolume));

    double currentVolume;
    EXPECT_TRUE(m_sut->getVolume(currentVolume));
    EXPECT_EQ(currentVolume, kVolume);
}

TEST_F(GstGenericPlayerTest, shouldGetVolumeWithPositiveFadeVolume)
{
    getContext([&](GenericPlayerContext &m_context) { m_context.audioFadeEnabled = true; });
    setPipelineState(GST_STATE_PLAYING);

    const gint kFadeVolume{70};
    const std::string kPropertyStr{"fade-volume"};

    expectGetSink(kAudioSinkStr, m_element);

    willGetElementProperty(kPropertyStr, kFadeVolume);

    double currentVolume;
    EXPECT_TRUE(m_sut->getVolume(currentVolume));
    EXPECT_DOUBLE_EQ(currentVolume, kFadeVolume / 100.0);
}

TEST_F(GstGenericPlayerTest, shouldRenderFrame)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createRenderFrame(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->renderFrame();
}

TEST_F(GstGenericPlayerTest, shouldFailToReturnVideoMute)
{
    bool resultMute{};
    EXPECT_FALSE(m_sut->getMute(MediaSourceType::VIDEO, resultMute));
}

TEST_F(GstGenericPlayerTest, shouldFailToReturnAudioMuteWhenPipelineIsNull)
{
    bool resultMute{};
    GstElement *pipelineTemp{nullptr};

    getContext(
        [&](GenericPlayerContext &m_context)
        {
            pipelineTemp = m_context.pipeline;
            m_context.pipeline = nullptr;
        });
    EXPECT_FALSE(m_sut->getMute(MediaSourceType::AUDIO, resultMute));

    // For teardown
    getContext([&](GenericPlayerContext &m_context) { m_context.pipeline = pipelineTemp; });
}

TEST_F(GstGenericPlayerTest, shouldReturnAudioMute)
{
    constexpr bool kMute{false};
    bool resultMute{};
    GstStreamVolume *volume = nullptr;

    getContext([&](const GenericPlayerContext &m_context) { volume = GST_STREAM_VOLUME(m_context.pipeline); });
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeGetMute(volume)).WillOnce(Return(kMute));
    EXPECT_TRUE(m_sut->getMute(MediaSourceType::AUDIO, resultMute));
    EXPECT_EQ(resultMute, kMute);
}

TEST_F(GstGenericPlayerTest, shouldFailToReturnSubtitleMuteWhenSinkIsNull)
{
    bool resultMute{};

    EXPECT_FALSE(m_sut->getMute(MediaSourceType::SUBTITLE, resultMute));
}

TEST_F(GstGenericPlayerTest, shouldReturnSubtitleMute)
{
    GstElement subtitleSink{};
    constexpr bool kMute{false};
    bool resultMute{};

    getContext([&](GenericPlayerContext &m_context) { m_context.subtitleSink = &subtitleSink; });
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&subtitleSink, StrEq("mute"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *ptr)
            {
                gboolean *responsePtr = reinterpret_cast<gboolean *>(ptr);
                *responsePtr = kMute;
            }));
    EXPECT_TRUE(m_sut->getMute(MediaSourceType::SUBTITLE, resultMute));
    EXPECT_EQ(resultMute, kMute);

    // in teardown
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&subtitleSink));
}

TEST_F(GstGenericPlayerTest, shouldMute)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetMute(_, _, MediaSourceType::AUDIO, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setMute(MediaSourceType::AUDIO, true);
}

TEST_F(GstGenericPlayerTest, shouldSetTextTrackIdentifier)
{
    const std::string kTextTrackIdentifier{"Identifier"};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetTextTrackIdentifier(_, kTextTrackIdentifier))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->setTextTrackIdentifier(kTextTrackIdentifier);
}

TEST_F(GstGenericPlayerTest, shouldFailToGetTextTrackIdentifierWhenSinkIsNull)
{
    std::string textTrackIdentifier;
    EXPECT_FALSE(m_sut->getTextTrackIdentifier(textTrackIdentifier));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetTextTrackIdentifierWhenSinkReturnsNull)
{
    GstElement subtitleSink{};
    std::string textTrackIdentifier;

    getContext([&](GenericPlayerContext &m_context) { m_context.subtitleSink = &subtitleSink; });
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&subtitleSink, StrEq("text-track-identifier"), _));
    EXPECT_FALSE(m_sut->getTextTrackIdentifier(textTrackIdentifier));

    // in teardown
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&subtitleSink));
}

TEST_F(GstGenericPlayerTest, shouldGetTextTrackIdentifier)
{
    GstElement subtitleSink{};
    std::string resultTextTrackIdentifier;
    std::string textTrackIdentifier{"Identifier"};

    getContext([&](GenericPlayerContext &m_context) { m_context.subtitleSink = &subtitleSink; });
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&subtitleSink, StrEq("text-track-identifier"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *ptr)
            {
                gchar **responsePtr = reinterpret_cast<gchar **>(ptr);
                *responsePtr = textTrackIdentifier.data();
            }));
    EXPECT_CALL(*m_glibWrapperMock, gFree(textTrackIdentifier.data()));
    EXPECT_TRUE(m_sut->getTextTrackIdentifier(resultTextTrackIdentifier));
    EXPECT_EQ(textTrackIdentifier, resultTextTrackIdentifier);

    // in teardown
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&subtitleSink));
}

TEST_F(GstGenericPlayerTest, shouldSetLowLatency)
{
    constexpr bool kLowLatency{true};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetLowLatency(_, _, kLowLatency)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setLowLatency(kLowLatency);
}

TEST_F(GstGenericPlayerTest, shouldSetSync)
{
    constexpr bool kSync{true};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetSync(_, _, kSync)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setSync(kSync);
}

TEST_F(GstGenericPlayerTest, shouldGetSync)
{
    const bool kSyncValue{true};
    const std::string kPropertyStr{"sync"};

    expectGetSink(kAudioSinkStr, m_element);
    willGetElementProperty(kPropertyStr, kSyncValue);

    bool sync;
    EXPECT_TRUE(m_sut->getSync(sync));
    EXPECT_EQ(sync, kSyncValue);
}

TEST_F(GstGenericPlayerTest, shouldGetPendingSyncIfNoSinkAvailable)
{
    const bool kSyncValue{true};

    getContext([&](GenericPlayerContext &m_context) { m_context.pendingSync = kSyncValue; });
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(kAudioSinkStr), _)).Times(1);

    bool sync;
    EXPECT_TRUE(m_sut->getSync(sync));
    EXPECT_EQ(sync, kSyncValue);
}

TEST_F(GstGenericPlayerTest, shouldFailToGetSyncIfStubNull)
{
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(kAudioSinkStr), _)).Times(1);

    bool sync;
    EXPECT_FALSE(m_sut->getSync(sync));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetSyncIfPropertyDoesntExist)
{
    expectGetSink(kAudioSinkStr, m_element);

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("sync"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);

    bool sync;
    EXPECT_FALSE(m_sut->getSync(sync));
}

TEST_F(GstGenericPlayerTest, shouldSetSyncOff)
{
    constexpr bool kSyncOff{true};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetSyncOff(_, _, kSyncOff)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setSyncOff(kSyncOff);
}

TEST_F(GstGenericPlayerTest, shouldSetStreamSyncMode)
{
    constexpr int32_t kStreamSyncMode{1};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetStreamSyncMode(_, _, MediaSourceType::AUDIO, kStreamSyncMode))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->setStreamSyncMode(MediaSourceType::AUDIO, kStreamSyncMode);
}

TEST_F(GstGenericPlayerTest, shouldGetStreamSyncMode)
{
    const int32_t kStreamSyncModeValue{1};
    const std::string kPropertyStr{"stream-sync-mode"};

    expectGetDecoder(m_element);
    willGetElementProperty(kPropertyStr, kStreamSyncModeValue);

    int32_t streamSyncMode;
    EXPECT_TRUE(m_sut->getStreamSyncMode(streamSyncMode));
    EXPECT_EQ(streamSyncMode, kStreamSyncModeValue);
}

TEST_F(GstGenericPlayerTest, shouldGetPendingStreamSyncModeIfNoSinkAvailable)
{
    constexpr int32_t kStreamSyncModeValue{1};

    getContext([&](GenericPlayerContext &m_context)
               { m_context.pendingStreamSyncMode[MediaSourceType::AUDIO] = kStreamSyncModeValue; });
    expectNoDecoder();

    int32_t streamSyncMode;
    EXPECT_TRUE(m_sut->getStreamSyncMode(streamSyncMode));
    EXPECT_EQ(streamSyncMode, kStreamSyncModeValue);
}

TEST_F(GstGenericPlayerTest, shouldGetStreamSyncModeWithIteratorResync)
{
    const int32_t kStreamSyncModeValue{1};
    const std::string kPropertyStr{"stream-sync-mode"};

    EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(_)).WillOnce(Return(&m_it));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(_, _))
        .WillOnce(Return(GST_ITERATOR_RESYNC))
        .WillOnce(Return(GST_ITERATOR_OK));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorResync(&m_it));
    EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(m_element));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(m_element)).WillOnce(Return(m_factory));
    EXPECT_CALL(*m_gstWrapperMock,
                gstElementFactoryListIsType(_, (GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO)))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_element)).WillOnce(Return(m_element));
    EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(_));

    willGetElementProperty(kPropertyStr, kStreamSyncModeValue);

    int32_t streamSyncMode;
    EXPECT_TRUE(m_sut->getStreamSyncMode(streamSyncMode));
    EXPECT_EQ(streamSyncMode, kStreamSyncModeValue);
}

TEST_F(GstGenericPlayerTest, shouldFailToGetStreamSyncModeIfNoDecoder)
{
    getContext([&](const GenericPlayerContext &m_context) { m_pipeline = m_context.pipeline; });

    expectNoDecoder();

    int32_t streamSyncMode;
    EXPECT_FALSE(m_sut->getStreamSyncMode(streamSyncMode));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetStreamSyncModeIfPropertyDoesntExist)
{
    expectGetDecoder(m_element);

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("stream-sync-mode"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);

    int32_t streamSyncMode;
    EXPECT_FALSE(m_sut->getStreamSyncMode(streamSyncMode));
}

TEST_F(GstGenericPlayerTest, shouldPing)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPing(_)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->ping(std::make_unique<StrictMock<firebolt::rialto::server::HeartbeatHandlerMock>>());
}

TEST_F(GstGenericPlayerTest, shouldFlush)
{
    constexpr bool kResetTime{true};
    bool isAsync{true};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(m_flushWatcherMock, setFlushing(MediaSourceType::VIDEO, isAsync));
    expectGetSink(kVideoSinkStr, m_element);
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("async"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *val)
            {
                gboolean *returnVal = reinterpret_cast<gboolean *>(val);
                *returnVal = TRUE;
            }));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element));
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createFlush(_, _, MediaSourceType::VIDEO, kResetTime))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->flush(MediaSourceType::VIDEO, kResetTime, isAsync);
}

TEST_F(GstGenericPlayerTest, shouldSetSourcePosition)
{
    constexpr int64_t kPosition{1234};
    constexpr bool kResetTime{true};
    constexpr double kAppliedRate{2.0};
    constexpr uint64_t kStopPosition{2352};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock,
                createSetSourcePosition(_, MediaSourceType::AUDIO, kPosition, kResetTime, kAppliedRate, kStopPosition))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->setSourcePosition(MediaSourceType::AUDIO, kPosition, kResetTime, kAppliedRate, kStopPosition);
}

TEST_F(GstGenericPlayerTest, shouldProcessAudioGap)
{
    constexpr int64_t kPosition{3};
    constexpr uint64_t kDuration{2};
    constexpr int64_t kDiscontinuityGap{1};
    constexpr bool kIsAudioAac{false};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createProcessAudioGap(_, kPosition, kDuration, kDiscontinuityGap, kIsAudioAac))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->processAudioGap(kPosition, kDuration, kDiscontinuityGap, kIsAudioAac);
}

TEST_F(GstGenericPlayerTest, shouldResetSourceAndSubtitleSinkOnTeardownWhenSet)
{
    GstElement source{};
    GstElement subtitleSink{};

    getContext(
        [&](GenericPlayerContext &m_context)
        {
            m_context.subtitleSink = &subtitleSink;
            m_context.source = &source;
        });
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&source));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&subtitleSink));
}

TEST_F(GstGenericPlayerTest, shouldSetBufferingLimit)
{
    constexpr std::uint32_t kBufferingLimit{123};

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetBufferingLimit(_, _, kBufferingLimit)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setBufferingLimit(kBufferingLimit);
}

TEST_F(GstGenericPlayerTest, shouldGetBufferingLimit)
{
    const uint32_t kBufferingLimit{1};
    const std::string kPropertyStr{"limit-buffering-ms"};

    expectGetDecoder(m_element);
    willGetElementProperty(kPropertyStr, kBufferingLimit);

    uint32_t bufferingLimit;
    EXPECT_TRUE(m_sut->getBufferingLimit(bufferingLimit));
    EXPECT_EQ(bufferingLimit, kBufferingLimit);
}

TEST_F(GstGenericPlayerTest, shouldGetPendingBufferingLimitIfNoSinkAvailable)
{
    constexpr uint32_t kBufferingLimitValue{1};

    getContext([&](GenericPlayerContext &m_context) { m_context.pendingBufferingLimit = kBufferingLimitValue; });
    expectNoDecoder();

    uint32_t bufferingLimit;
    EXPECT_TRUE(m_sut->getBufferingLimit(bufferingLimit));
    EXPECT_EQ(bufferingLimit, kBufferingLimitValue);
}

TEST_F(GstGenericPlayerTest, shouldGetBufferingLimitWithIteratorResync)
{
    const uint32_t kBufferingLimitValue{1};
    const std::string kPropertyStr{"limit-buffering-ms"};

    EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(_)).WillOnce(Return(&m_it));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(_, _))
        .WillOnce(Return(GST_ITERATOR_RESYNC))
        .WillOnce(Return(GST_ITERATOR_OK));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorResync(&m_it));
    EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(m_element));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(m_element)).WillOnce(Return(m_factory));
    EXPECT_CALL(*m_gstWrapperMock,
                gstElementFactoryListIsType(_, (GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO)))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_element)).WillOnce(Return(m_element));
    EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(_));

    willGetElementProperty(kPropertyStr, kBufferingLimitValue);

    uint32_t bufferingLimit;
    EXPECT_TRUE(m_sut->getBufferingLimit(bufferingLimit));
    EXPECT_EQ(bufferingLimit, kBufferingLimitValue);
}

TEST_F(GstGenericPlayerTest, shouldFailToGetBufferingLimitIfNoDecoder)
{
    getContext([&](const GenericPlayerContext &m_context) { m_pipeline = m_context.pipeline; });

    expectNoDecoder();

    uint32_t bufferingLimit;
    EXPECT_FALSE(m_sut->getBufferingLimit(bufferingLimit));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetBufferingLimitIfPropertyDoesntExist)
{
    expectGetDecoder(m_element);

    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("limit-buffering-ms"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_element)).Times(1);

    uint32_t bufferingLimit;
    EXPECT_FALSE(m_sut->getBufferingLimit(bufferingLimit));
}

TEST_F(GstGenericPlayerTest, shouldSetUseBuffering)
{
    constexpr bool kUseBuffering{true};

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetUseBuffering(_, _, kUseBuffering)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setUseBuffering(kUseBuffering);
}

TEST_F(GstGenericPlayerTest, shouldGetUseBuffering)
{
    const bool kUseBuffering{true};
    const std::string kPropertyStr{"use-buffering"};
    GstElement decodebin{};

    getContext(
        [&](GenericPlayerContext &m_context)
        {
            m_context.pendingUseBuffering = kUseBuffering;
            m_context.playbackGroup.m_curAudioDecodeBin = &decodebin;
        });
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&decodebin, StrEq(kPropertyStr.c_str()), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *val)
            {
                gint *returnVal = reinterpret_cast<gint *>(val);
                *returnVal = kUseBuffering;
            }));

    bool useBuffering{false};
    EXPECT_TRUE(m_sut->getUseBuffering(useBuffering));
    EXPECT_EQ(useBuffering, kUseBuffering);
}

TEST_F(GstGenericPlayerTest, shouldGetPendingUseBufferingIfNoDecodebinAvailable)
{
    constexpr bool kUseBufferingValue{true};

    getContext([&](GenericPlayerContext &m_context) { m_context.pendingUseBuffering = kUseBufferingValue; });

    bool useBuffering{false};
    EXPECT_TRUE(m_sut->getUseBuffering(useBuffering));
    EXPECT_EQ(useBuffering, kUseBufferingValue);
}

TEST_F(GstGenericPlayerTest, shouldFailToGetUseBufferingIfNoDecodebin)
{
    bool useBuffering{false};
    EXPECT_FALSE(m_sut->getUseBuffering(useBuffering));
}

TEST_F(GstGenericPlayerTest, shouldSwitchSource)
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/mpeg");

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSwitchSource(_, Ref(source))).WillOnce(Return(ByMove(std::move(task))));

    m_sut->switchSource(source);
}
