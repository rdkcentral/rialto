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
#include "GstPlayerTestCommon.h"
#include "IMediaPipeline.h"
#include "Matchers.h"
#include "MediaSourceUtil.h"
#include "PlayerTaskMock.h"

using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Return;

class GstPlayerTest : public GstPlayerTestCommon
{
protected:
    std::unique_ptr<IGstPlayer> m_sut;
    VideoRequirements m_videoReq = {};

    GstPlayerTest()
    {
        gstPlayerWillBeCreated();
        m_sut = std::make_unique<GstPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, MediaType::MSE, m_videoReq,
                                            m_gstWrapperMock, m_glibWrapperMock, m_gstSrcFactoryMock,
                                            m_timerFactoryMock, std::move(taskFactory), std::move(workerThreadFactory),
                                            std::move(gstDispatcherThreadFactory));
    }

    ~GstPlayerTest() override
    {
        gstPlayerWillBeDestroyed();
        m_sut.reset();
    }
};

TEST_F(GstPlayerTest, shouldAttachSource)
{
    firebolt::rialto::IMediaPipeline::MediaSource source{-1, firebolt::rialto::MediaSourceType::VIDEO, "video/mpeg"};

    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createAttachSource(_, source)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->attachSource(source);
}

TEST_F(GstPlayerTest, shouldPlay)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPlay(_)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->play();
}

TEST_F(GstPlayerTest, shouldPause)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createPause(_)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->pause();
}

TEST_F(GstPlayerTest, shouldStop)
{
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createStop(_, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->stop();
}

TEST_F(GstPlayerTest, shouldAttachSamplesFromVector)
{
    IMediaPipeline::MediaSegmentVector mediaSegments;
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createAttachSamples(_, _, _)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->attachSamples(mediaSegments);
}

TEST_F(GstPlayerTest, shouldAttachSamplesFromShm)
{
    std::shared_ptr<IDataReader> dataReader{std::make_shared<DataReaderMock>()};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createReadShmDataAndAttachSamples(_, _, dataReader))
        .WillOnce(Return(ByMove(std::move(task))));

    m_sut->attachSamples(dataReader);
}

TEST_F(GstPlayerTest, shouldSetPlaybackRate)
{
    double playbackRate{1.5};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetPlaybackRate(_, playbackRate)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setPlaybackRate(playbackRate);
}

TEST_F(GstPlayerTest, shouldSetPosition)
{
    std::int64_t position{12345};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetPosition(_, _, position)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setPosition(position);
}

TEST_F(GstPlayerTest, shouldSetVideoGeometry)
{
    Rectangle rectangle{1, 2, 3, 4};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createSetVideoGeometry(_, _, rectangle)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setVideoGeometry(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
}

TEST_F(GstPlayerTest, shouldSetEos)
{
    MediaSourceType type{MediaSourceType::AUDIO};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(m_taskFactoryMock, createEos(_, _, type)).WillOnce(Return(ByMove(std::move(task))));

    m_sut->setEos(type);
}

TEST_F(GstPlayerTest, shouldSetupSource)
{
    GstElement source{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(&source));
    EXPECT_CALL(m_taskFactoryMock, createSetupSource(_, _, &source)).WillOnce(Return(ByMove(std::move(task))));

    triggerSetupSource(&source);
}

TEST_F(GstPlayerTest, shouldSetupElement)
{
    GstElement element{};
    std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*task), execute());
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(&element));
    EXPECT_CALL(m_taskFactoryMock, createSetupElement(_, _, &element)).WillOnce(Return(ByMove(std::move(task))));

    triggerSetupElement(&element);
}

TEST_F(GstPlayerTest, shouldReturnInvalidPositionWhenPipelineIsBelowPlayingState)
{
    int64_t targetPosition{};
    EXPECT_FALSE(m_sut->getPosition(targetPosition));
}

TEST_F(GstPlayerTest, shouldReturnInvalidPositionWhenQueryFails)
{
    int64_t targetPosition{};
    setPipelineState(GST_STATE_PLAYING);
    EXPECT_CALL(*m_gstWrapperMock, gstElementQueryPosition(_, GST_FORMAT_TIME, _)).WillOnce(Return(FALSE));
    EXPECT_FALSE(m_sut->getPosition(targetPosition));
}

TEST_F(GstPlayerTest, shouldReturnPosition)
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
