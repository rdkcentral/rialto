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

#include "DecryptionServiceMock.h"
#include "GlibWrapperMock.h"
#include "GstDispatcherThreadFactoryMock.h"
#include "GstDispatcherThreadMock.h"
#include "GstPlayer.h"
#include "GstPlayerClientMock.h"
#include "GstSrcFactoryMock.h"
#include "GstSrcMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "PlayerTaskFactoryMock.h"
#include "PlayerTaskMock.h"
#include "TimerFactoryMock.h"
#include "WorkerThreadFactoryMock.h"
#include "WorkerThreadMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;

class RialtoServerCreateGstPlayerTest : public ::testing::Test
{
protected:
    std::unique_ptr<IGstPlayer> m_gstPlayer;
    StrictMock<GstPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock{std::make_shared<StrictMock<GlibWrapperMock>>()};
    std::shared_ptr<StrictMock<GstSrcFactoryMock>> m_gstSrcFactoryMock{std::make_shared<StrictMock<GstSrcFactoryMock>>()};
    std::shared_ptr<StrictMock<GstSrcMock>> m_gstSrcMock{std::make_shared<StrictMock<GstSrcMock>>()};
    std::shared_ptr<StrictMock<TimerFactoryMock>> m_timerFactoryMock{std::make_shared<StrictMock<TimerFactoryMock>>()};
    std::unique_ptr<IPlayerTaskFactory> m_taskFactory{std::make_unique<StrictMock<PlayerTaskFactoryMock>>()};
    StrictMock<PlayerTaskFactoryMock> &m_taskFactoryMock{
        dynamic_cast<StrictMock<PlayerTaskFactoryMock> &>(*m_taskFactory)};
    std::unique_ptr<IWorkerThreadFactory> workerThreadFactory{std::make_unique<StrictMock<WorkerThreadFactoryMock>>()};
    StrictMock<WorkerThreadFactoryMock> &m_workerThreadFactoryMock{
        dynamic_cast<StrictMock<WorkerThreadFactoryMock> &>(*workerThreadFactory)};
    std::unique_ptr<IWorkerThread> workerThread{std::make_unique<StrictMock<WorkerThreadMock>>()};
    StrictMock<WorkerThreadMock> &m_workerThreadMock{dynamic_cast<StrictMock<WorkerThreadMock> &>(*workerThread)};
    std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory{
        std::make_unique<StrictMock<GstDispatcherThreadFactoryMock>>()};
    StrictMock<GstDispatcherThreadFactoryMock> &m_gstDispatcherThreadFactoryMock{
        dynamic_cast<StrictMock<GstDispatcherThreadFactoryMock> &>(*gstDispatcherThreadFactory)};
    std::unique_ptr<IGstDispatcherThread> gstDispatcherThread{std::make_unique<StrictMock<GstDispatcherThreadMock>>()};
    StrictMock<GstDispatcherThreadMock> &m_gstDispatcherThreadMock{
        dynamic_cast<StrictMock<GstDispatcherThreadMock> &>(*gstDispatcherThread)};
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;

    MediaType m_type{MediaType::MSE};
    GstElement m_pipeline{};
    GFlagsClass m_flagsClass{};
    GstElement m_playsink{};
    GstBus m_bus{};
    GType m_gstPlayFlagsType{static_cast<GType>(123)};
    GFlagsValue m_audioFlag{1, "audio", "audio"};
    GFlagsValue m_videoFlag{2, "video", "video"};
    GFlagsValue m_nativeVideoFlag{3, "native-video", "native-video"};
    VideoRequirements m_videoReq = {MIN_PRIMARY_VIDEO_WIDTH, MIN_PRIMARY_VIDEO_HEIGHT};
    PlayerContext m_storedPlayerContext;

    void initFactories() { EXPECT_CALL(*m_gstSrcFactoryMock, getGstSrc()).WillOnce(Return(m_gstSrcMock)); }

    void expectMakePlaybin()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("playbin"), _)).WillOnce(Return(&m_pipeline));
    }

    void expectSetFlags()
    {
        EXPECT_CALL(*m_glibWrapperMock, gTypeFromName(CharStrMatcher("GstPlayFlags")))
            .Times(3)
            .WillRepeatedly(Return(m_gstPlayFlagsType));
        EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(m_gstPlayFlagsType)).Times(3).WillRepeatedly(Return(&m_flagsClass));

        EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, CharStrMatcher("audio")))
            .WillOnce(Return(&m_audioFlag));
        EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, CharStrMatcher("video")))
            .WillOnce(Return(&m_videoFlag));
        EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, CharStrMatcher("native-video")))
            .WillOnce(Return(&m_nativeVideoFlag));

        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, CharStrMatcher("flags")));
    }

    void expectSetSignalCallbacks()
    {
        EXPECT_CALL(*m_glibWrapperMock,
                    gSignalConnect(&m_pipeline, CharStrMatcher("source-setup"), NotNullMatcher(), NotNullMatcher()));
        EXPECT_CALL(*m_glibWrapperMock,
                    gSignalConnect(&m_pipeline, CharStrMatcher("element-setup"), NotNullMatcher(), NotNullMatcher()));
    }

    void expectSetUri() { EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, CharStrMatcher("uri"))); }

    void expectCheckPlaySink()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(GST_BIN(&m_pipeline), CharStrMatcher("playsink")))
            .WillOnce(Return(&m_playsink));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_playsink, CharStrMatcher("send-event-mode")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&m_playsink));
    }

    void expectSetMessageCallback()
    {
        EXPECT_CALL(m_gstDispatcherThreadFactoryMock, createGstDispatcherThread(_, _, _, _, _))
            .WillOnce(DoAll(SaveArg<0>(&m_storedPlayerContext), Return(ByMove(std::move(gstDispatcherThread)))));
    }

    void expectGstPlayerShutdown()
    {
        std::unique_ptr<IPlayerTask> stopTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
        std::unique_ptr<IPlayerTask> shutdownTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
        EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*stopTask), execute());
        EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*shutdownTask), execute());
        EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
            .WillRepeatedly(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
        EXPECT_CALL(m_workerThreadMock, join());
        EXPECT_CALL(m_taskFactoryMock, createShutdown(_)).WillOnce(Return(ByMove(std::move(shutdownTask))));
        EXPECT_CALL(m_taskFactoryMock, createStop(_, _)).WillOnce(Return(ByMove(std::move(stopTask))));
        EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
        EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus));
    }

    void createGstPlayerSuccess()
    {
        initFactories();

        expectMakePlaybin();
        expectSetFlags();
        expectSetSignalCallbacks();
        expectSetUri();
        expectCheckPlaySink();
        expectSetMessageCallback();

        EXPECT_CALL(*m_gstSrcMock, initSrc());
        EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));

        EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  m_gstSrcFactoryMock, m_timerFactoryMock,
                                                                  std::move(m_taskFactory), std::move(workerThreadFactory),
                                                                  std::move(gstDispatcherThreadFactory)););
        EXPECT_NE(m_gstPlayer, nullptr);
    }

    void destroyGstPlayerSuccess()
    {
        expectGstPlayerShutdown();

        // GlibPlayer shall unref the pipeline on destruction
        EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&m_pipeline));

        m_gstPlayer.reset();
    }
};

/**
 * Test that a GstPlayer object can be created successfully if the video requirements are equal to or greater than the minimum.
 */
TEST_F(RialtoServerCreateGstPlayerTest, CreateDestroyPrimaryVideoSuccess)
{
    createGstPlayerSuccess();
    EXPECT_EQ(false, m_storedPlayerContext.isSecondaryVideo);

    destroyGstPlayerSuccess();
}

/**
 * Test that a GstPlayer object can be created successfully for a secondary video if width is less than the minimum.
 */
TEST_F(RialtoServerCreateGstPlayerTest, CreateDestroySecondaryVideoMinWidthSuccess)
{
    // Width < minimum
    m_videoReq.maxWidth = MIN_PRIMARY_VIDEO_WIDTH - 1;
    m_videoReq.maxHeight = MIN_PRIMARY_VIDEO_HEIGHT;
    createGstPlayerSuccess();
    EXPECT_EQ(true, m_storedPlayerContext.isSecondaryVideo);

    destroyGstPlayerSuccess();
}

/**
 * Test that a GstPlayer object can be created successfully for a secondary video if height is less than the minimum.
 */
TEST_F(RialtoServerCreateGstPlayerTest, CreateDestroySecondaryVideoMinHeightSuccess)
{
    // Height < minimum
    m_videoReq.maxWidth = MIN_PRIMARY_VIDEO_WIDTH;
    m_videoReq.maxHeight = MIN_PRIMARY_VIDEO_HEIGHT - 1;
    createGstPlayerSuccess();
    EXPECT_EQ(true, m_storedPlayerContext.isSecondaryVideo);

    destroyGstPlayerSuccess();
}

/**
 * Test that a GstPlayer throws an exception if GstSrcFactory is null.
 */
TEST_F(RialtoServerCreateGstPlayerTest, GstSrcFactoryNull)
{
    EXPECT_THROW(m_gstPlayer = std::make_unique<GstPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                           m_videoReq, m_gstWrapperMock, m_glibWrapperMock, nullptr,
                                                           m_timerFactoryMock, std::move(m_taskFactory),
                                                           std::move(workerThreadFactory),
                                                           std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstPlayer throws an exception if TimerFactory is invalid
 */
TEST_F(RialtoServerCreateGstPlayerTest, TimerFactoryFails)
{
    initFactories();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                           m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                           m_gstSrcFactoryMock, nullptr, std::move(m_taskFactory),
                                                           std::move(workerThreadFactory),
                                                           std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstPlayer throws an exception if GstSrcFactory fails to create GstSrc.
 */
TEST_F(RialtoServerCreateGstPlayerTest, GstSrcFactoryFails)
{
    EXPECT_CALL(*m_gstSrcFactoryMock, getGstSrc()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                           m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                           m_gstSrcFactoryMock, m_timerFactoryMock,
                                                           std::move(m_taskFactory), std::move(workerThreadFactory),
                                                           std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstPlayer throws an exception an unknown media type is used.
 */
TEST_F(RialtoServerCreateGstPlayerTest, UnknownMediaType)
{
    EXPECT_CALL(*m_gstSrcFactoryMock, getGstSrc()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstPlayer>(&m_gstPlayerClient, m_decryptionServiceMock,
                                                           MediaType::UNKNOWN, m_videoReq, m_gstWrapperMock,
                                                           m_glibWrapperMock, m_gstSrcFactoryMock, m_timerFactoryMock,
                                                           std::move(m_taskFactory), std::move(workerThreadFactory),
                                                           std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstPlayer is still created if playsink not found.
 */
TEST_F(RialtoServerCreateGstPlayerTest, PlaysinkNotFound)
{
    initFactories();

    expectMakePlaybin();
    expectSetFlags();
    expectSetSignalCallbacks();
    expectSetUri();
    expectSetMessageCallback();

    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));

    EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(GST_BIN(&m_pipeline), CharStrMatcher("playsink")))
        .WillOnce(Return(nullptr));

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                              m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                              m_gstSrcFactoryMock, m_timerFactoryMock,
                                                              std::move(m_taskFactory), std::move(workerThreadFactory),
                                                              std::move(gstDispatcherThreadFactory)););
    EXPECT_NE(m_gstPlayer, nullptr);

    expectGstPlayerShutdown();

    // GlibPlayer shall unref the pipeline on destruction
    EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&m_pipeline));
}
