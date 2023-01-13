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
#include "IMediaPipeline.h"
#include "Matchers.h"
#include "MediaSourceUtil.h"
#include "PlayerTaskMock.h"

using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Ref;
using testing::Return;

class GstGenericPlayerTest : public GstGenericPlayerTestCommon
{
protected:
    std::unique_ptr<IGstGenericPlayer> m_sut;
    VideoRequirements m_videoReq = {};

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
};

TEST_F(GstGenericPlayerTest, shouldAttachSource)
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>(-1, "video/mpeg");

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createAttachSource(_, _, Ref(source))).WillOnce(Return(ByMove(std::move(task))));

    m_sut->attachSource(source);
}

TEST_F(GstGenericPlayerTest, shouldRemoveSource)
{
    constexpr std::uint32_t audioSourceId{1};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createRemoveSource(_, MediaSourceType::AUDIO)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->removeSource(audioSourceId);
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
    EXPECT_CALL(m_taskFactoryMock, createPause(_)).WillOnce(Return(ByMove(std::move(task))));

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

TEST_F(GstPlayerTest, shouldAddDeepElement)
{
    GstElement element{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createDeepElementAdded(_, _, _, &element)).WillOnce(Return(ByMove(std::move(task))));

    triggerDeepElementAdded(&element);
}

TEST_F(GstPlayerTest, shouldReturnInvalidPositionWhenPipelineIsBelowPlayingState)
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

TEST_F(GstGenericPlayerTest, shouldReturnPosition)
{
    constexpr gint64 expectedPosition{123};
    int64_t targetPosition{};
    setPipelineState(GST_STATE_PLAYING);
    EXPECT_CALL(*m_gstWrapperMock, gstElementQueryPosition(_, GST_FORMAT_TIME, _))
        .WillOnce(Invoke(
            [&](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = expectedPosition;
                return TRUE;
            }));
    EXPECT_TRUE(m_sut->getPosition(targetPosition));
    EXPECT_EQ(expectedPosition, targetPosition);
}

TEST_F(GstGenericPlayerTest, shouldRenderFrame)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createRenderFrame(_)).WillOnce(Return(ByMove(std::move(task))));

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
