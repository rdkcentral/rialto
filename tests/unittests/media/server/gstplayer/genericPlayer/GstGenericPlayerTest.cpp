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
#include "IMediaPipeline.h"
#include "MatchersGenericPlayer.h"
#include "MediaSourceUtil.h"
#include "PlayerTaskMock.h"

using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Ref;
using testing::Return;
using testing::StrEq;
using testing::DoAll;
using testing::SetArgumentPointee;

class GstGenericPlayerTest : public GstGenericPlayerTestCommon
{
protected:
    std::unique_ptr<IGstGenericPlayer> m_sut;
    VideoRequirements m_videoReq = {kMinPrimaryVideoWidth, kMinPrimaryVideoHeight};

    GstGenericPlayerTest()
    {
        gstPlayerWillBeCreated();
        m_sut = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, MediaType::MSE,
                                                   m_videoReq, m_gstWrapperMock, m_glibWrapperMock, m_gstSrcFactoryMock,
                                                   m_timerFactoryMock, std::move(m_taskFactory),
                                                   std::move(workerThreadFactory), std::move(gstDispatcherThreadFactory),
                                                   m_gstProtectionMetadataFactoryMock);
    }

    ~GstGenericPlayerTest() override
    {
        gstPlayerWillBeDestroyed();
        m_sut.reset();
    }

    void getContext(const std::function<void(GenericPlayerContext &)> &fun)
    {
        // Call any method to modify GstGenericPlayer context
        double volume{};
        std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
        EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
        EXPECT_CALL(m_taskFactoryMock, createSetVolume(_, volume))
            .WillOnce(Invoke(
                [&](GenericPlayerContext &m_context, double volume)
                {
                    fun(m_context);
                    return std::move(task);
                }));
        m_sut->setVolume(volume);
    }

    GstElement *initRealElement()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        GstElement *element = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
        return element;
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

TEST_F(GstGenericPlayerTest, shouldPlay)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPlay(_)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->play();
}

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

TEST_F(GstGenericPlayerTest, shouldReturnInvalidPositionWhenPipelineIsBelowPausedState)
{
    int64_t targetPosition{};
    EXPECT_FALSE(m_sut->getPosition(targetPosition));
}

TEST_F(GstGenericPlayerTest, shouldReturnInvalidPositionWhenQueryFails)
{
    int64_t targetPosition{};
    setPipelineState(GST_STATE_PLAYING);
    EXPECT_CALL(*m_gstWrapperMock, gstElementQueryPosition(_, GST_FORMAT_TIME, _)).WillOnce(Return(FALSE));
    EXPECT_FALSE(m_sut->getPosition(targetPosition));
}

TEST_F(GstGenericPlayerTest, shouldReturnPositionInPlayingState)
{
    constexpr gint64 kExpectedPosition{123};
    int64_t targetPosition{};
    setPipelineState(GST_STATE_PLAYING);
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
    setPipelineState(GST_STATE_PAUSED);
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

TEST_F(GstGenericPlayerTest, shouldGetStatsInPlayingState)
{
    constexpr guint64 kRenderedFrames{1234};
    constexpr guint64 kDroppedFrames{5};
    uint64_t returnedRenderedFrames{};
    uint64_t returnedDroppedFrames{};
    GstElement *realElement;
    realElement = initRealElement();
    setPipelineState(GST_STATE_PLAYING);

    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("video-sink"), _))
        .WillOnce(Invoke(
                      [&](gpointer object, const gchar *first_property_name, void *element)
                          {
                              GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                              *elementPtr = realElement;
                          }));
    const std::string kElementTypeName{"GenericSink"};
    EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(realElement))).WillOnce(Return(kElementTypeName.c_str()));

    GstStructure testStructure;
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _))
        .WillOnce(Invoke(
                      [&](gpointer object, const gchar *first_property_name, void *element)
                          {
                              GstStructure **elementPtr = reinterpret_cast<GstStructure **>(element);
                              *elementPtr = &testStructure;
                          }));
    
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&testStructure, StrEq("rendered"), _)).WillOnce(DoAll(SetArgumentPointee<2>(kRenderedFrames), Return(true)));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&testStructure, StrEq("dropped"), _)).WillOnce(DoAll(SetArgumentPointee<2>(kDroppedFrames), Return(true)));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(realElement)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstStructureFree(&testStructure)).Times(1);

    EXPECT_TRUE(m_sut->getStats(MediaSourceType::VIDEO, returnedRenderedFrames, returnedDroppedFrames));
    EXPECT_EQ(kRenderedFrames, returnedRenderedFrames);
    EXPECT_EQ(kDroppedFrames, returnedDroppedFrames);
    
    gst_object_unref(realElement);
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
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("audio-sink"), _)).Times(1);
    
    uint64_t returnedRenderedFrames;
    uint64_t returnedDroppedFrames;
    EXPECT_FALSE(m_sut->getStats(MediaSourceType::AUDIO, returnedRenderedFrames, returnedDroppedFrames));
}

TEST_F(GstGenericPlayerTest, shouldFailToGetStatsInPlayingStateIfStructureNull)
{
    GstElement *realElement;
    realElement = initRealElement();
    setPipelineState(GST_STATE_PLAYING);

    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("video-sink"), _))
        .WillOnce(Invoke(
                      [&](gpointer object, const gchar *first_property_name, void *element)
                          {
                              GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                              *elementPtr = realElement;
                          }));
    const std::string kElementTypeName{"GenericSink"};
    EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(realElement))).WillOnce(Return(kElementTypeName.c_str()));

    // Fail to get GstStructure which should cause the getStats() call to return false
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _)).Times(1);
    
    uint64_t returnedRenderedFrames;
    uint64_t returnedDroppedFrames;
    EXPECT_FALSE(m_sut->getStats(MediaSourceType::VIDEO, returnedRenderedFrames, returnedDroppedFrames));

    gst_object_unref(realElement);
}

TEST_F(GstGenericPlayerTest, shouldFailToGetStatsInPlayingStateIfStructIncomplete)
{
    GstElement *realElement;
    realElement = initRealElement();
    setPipelineState(GST_STATE_PLAYING);

    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("video-sink"), _))
        .WillOnce(Invoke(
                      [&](gpointer object, const gchar *first_property_name, void *element)
                          {
                              GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                              *elementPtr = realElement;
                          }));
    const std::string kElementTypeName{"GenericSink"};
    EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(realElement))).WillOnce(Return(kElementTypeName.c_str()));

    GstStructure testStructure;
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _))
        .WillOnce(Invoke(
                      [&](gpointer object, const gchar *first_property_name, void *element)
                          {
                              GstStructure **elementPtr = reinterpret_cast<GstStructure **>(element);
                              *elementPtr = &testStructure;
                          }));
    
    EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&testStructure, StrEq("rendered"), _)).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(realElement)).Times(1);
    EXPECT_CALL(*m_gstWrapperMock, gstStructureFree(&testStructure)).Times(1);

    uint64_t returnedRenderedFrames;
    uint64_t returnedDroppedFrames;
    EXPECT_FALSE(m_sut->getStats(MediaSourceType::VIDEO, returnedRenderedFrames, returnedDroppedFrames));
    
    gst_object_unref(realElement);
}

TEST_F(GstGenericPlayerTest, shouldRenderFrame)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createRenderFrame(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->renderFrame();
}

TEST_F(GstGenericPlayerTest, shouldReturnVolume)
{
    constexpr double kVolume{0.7};
    double resultVolume{};
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeGetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR)).WillOnce(Return(kVolume));
    EXPECT_TRUE(m_sut->getVolume(resultVolume));
    EXPECT_EQ(resultVolume, kVolume);
}

TEST_F(GstGenericPlayerTest, shouldReturnMute)
{
    constexpr bool kMute{false};
    bool resultMute{};
    GstStreamVolume *volume = nullptr;

    getContext([&](GenericPlayerContext &m_context) { volume = GST_STREAM_VOLUME(m_context.pipeline); });
    EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeGetMute(volume)).WillOnce(Return(kMute));
    EXPECT_TRUE(m_sut->getMute(resultMute));
    EXPECT_EQ(resultMute, kMute);
}

TEST_F(GstGenericPlayerTest, shouldMute)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetMute(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setMute(true);
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
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createFlush(_, MediaSourceType::AUDIO, kResetTime))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->flush(MediaSourceType::AUDIO, kResetTime);
}

TEST_F(GstGenericPlayerTest, shouldSetSourcePosition)
{
    constexpr int64_t kPosition{1234};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetSourcePosition(_, MediaSourceType::AUDIO, kPosition))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->setSourcePosition(MediaSourceType::AUDIO, kPosition);
}
