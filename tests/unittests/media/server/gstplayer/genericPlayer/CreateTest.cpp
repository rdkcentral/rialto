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

#include "GlibWrapperFactoryMock.h"
#include "GstGenericPlayerTestCommon.h"
#include "GstWrapperFactoryMock.h"
#include "IFactoryAccessor.h"
#include "Matchers.h"
#include "PlayerTaskMock.h"
#include "RdkGstreamerUtilsWrapperFactoryMock.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrEq;
using ::testing::StrictMock;

class RialtoServerCreateGstGenericPlayerTest : public GstGenericPlayerTestCommon
{
protected:
    std::unique_ptr<IGstGenericPlayer> m_gstPlayer;

    MediaType m_type{MediaType::MSE};
    VideoRequirements m_videoReq = {kMinPrimaryVideoWidth, kMinPrimaryVideoHeight};
    GenericPlayerContext m_storedPlayerContext;
    GstObject m_westerosFactory{}; // GstElementFactory is an opaque data structure
    char m_dummyContext{};
    GstStructure m_contextStructure{};
    GstElement m_westerosSink{};
    GParamSpec m_rectangleSpec{};

    void expectCreatePipeline()
    {
        EXPECT_CALL(m_gstInitialiserMock, waitForInitialisation());
        initFactories();
        expectMakePlaybin();
        expectSetFlags();
        expectSetSignalCallbacks();
        expectSetUri();
        expectCheckPlaySink();

        EXPECT_CALL(*m_gstSrcMock, initSrc());
        EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
        EXPECT_CALL(*m_gstProtectionMetadataFactoryMock, createProtectionMetadataWrapper(_))
            .WillOnce(Return(ByMove(std::move(m_gstProtectionMetadataWrapper))));
    }

    void createGstGenericPlayerSuccess()
    {
        gstPlayerWillBeCreated();

        EXPECT_NO_THROW(
            m_gstPlayer =
                std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type, m_videoReq,
                                                   m_gstWrapperMock, m_glibWrapperMock, m_rdkGstreamerUtilsWrapperMock,
                                                   m_gstInitialiserMock, std::move(m_flushWatcher), m_gstSrcFactoryMock,
                                                   m_timerFactoryMock, std::move(m_taskFactory),
                                                   std::move(workerThreadFactory), std::move(gstDispatcherThreadFactory),
                                                   m_gstProtectionMetadataFactoryMock));
        EXPECT_NE(m_gstPlayer, nullptr);
    }

    void destroyGstGenericPlayerSuccess()
    {
        gstPlayerWillBeDestroyed();

        m_gstPlayer.reset();
    }

    void expectSetWesterosSecondaryVideo()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink")))
            .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_westerosFactory)));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_westerosFactory), _))
            .WillOnce(Return(&m_westerosSink));
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("res-usage"))).WillOnce(Return(&m_rectangleSpec));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, StrEq("res-usage")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, StrEq("video-sink")));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_westerosFactory)));
    }

    void expectSetContext()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstContextNew(StrEq("erm"), false))
            .WillOnce(Return(reinterpret_cast<GstContext *>(&m_dummyContext)));
        EXPECT_CALL(*m_gstWrapperMock, gstContextWritableStructure(reinterpret_cast<GstContext *>(&m_dummyContext)))
            .WillOnce(Return(&m_contextStructure));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstStructureSetUintStub(&m_contextStructure, StrEq("res-usage"), G_TYPE_UINT, 0x0u));
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetContext(_, reinterpret_cast<GstContext *>(&m_dummyContext)));
        EXPECT_CALL(*m_gstWrapperMock, gstContextUnref(reinterpret_cast<GstContext *>(&m_dummyContext)));
    }
};

/**
 * Test the factory
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, FactoryCreatesObject)
{
    // Testcase setup
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock{
        std::make_shared<StrictMock<GstWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<GlibWrapperFactoryMock>> m_glibWrapperFactoryMock{
        std::make_shared<StrictMock<GlibWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<RdkGstreamerUtilsWrapperFactoryMock>> m_rdkGstreamerUtilsWrapperFactoryMock{
        std::make_shared<StrictMock<RdkGstreamerUtilsWrapperFactoryMock>>()};
    IFactoryAccessor::instance().gstWrapperFactory() = m_gstWrapperFactoryMock;
    IFactoryAccessor::instance().glibWrapperFactory() = m_glibWrapperFactoryMock;
    IFactoryAccessor::instance().rdkGstreamerUtilsWrapperFactory() = m_rdkGstreamerUtilsWrapperFactoryMock;

    // Create expectations
    EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillRepeatedly(Return(m_gstWrapperMock));
    EXPECT_CALL(*m_glibWrapperFactoryMock, getGlibWrapper()).WillRepeatedly(Return(m_glibWrapperMock));
    EXPECT_CALL(*m_rdkGstreamerUtilsWrapperFactoryMock, createRdkGstreamerUtilsWrapper())
        .WillOnce(Return(m_rdkGstreamerUtilsWrapperMock));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("rialtosrc"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(0, StrEq("rialtosrc"), _, _));
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(_)).WillRepeatedly(Return(nullptr));
    expectMakePlaybin();
    expectSetFlags();
    expectSetSignalCallbacks();
    expectSetUri();
    expectCheckPlaySink();
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_READY))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));

    std::shared_ptr<firebolt::rialto::server::IGstGenericPlayerFactory> factory =
        firebolt::rialto::server::IGstGenericPlayerFactory::getFactory();
    ASSERT_NE(factory, nullptr);
    auto player{factory->createGstGenericPlayer(&m_gstPlayerClient, m_decryptionServiceMock, m_type, m_videoReq,
                                                m_rdkGstreamerUtilsWrapperFactoryMock)};
    EXPECT_NE(player, nullptr);

    // Destroy expectations
    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(nullptr, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(_, GST_STATE_NULL)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(_)).Times(2);
    player.reset();

    // Cleanup
    IFactoryAccessor::instance().gstWrapperFactory() = nullptr;
    IFactoryAccessor::instance().glibWrapperFactory() = nullptr;
    IFactoryAccessor::instance().rdkGstreamerUtilsWrapperFactory() = nullptr;
}

/**
 * Test that a GstGenericPlayer object can be created successfully if the video requirements are equal to or greater
 * than the minimum.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateDestroyPrimaryVideoSuccess)
{
    createGstGenericPlayerSuccess();

    destroyGstGenericPlayerSuccess();
}

/**
 * Test the factory
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, GetFactorySucceeds)
{
    std::shared_ptr<firebolt::rialto::server::IGstGenericPlayerFactory> factory =
        firebolt::rialto::server::IGstGenericPlayerFactory::getFactory();
    EXPECT_NE(factory, nullptr);
}

/**
 * Test that a GstGenericPlayer object can be created successfully for a secondary video if width is less than the minimum.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateDestroySecondaryVideoMinWidthSuccess)
{
    // Width < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth - 1;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight;
    expectSetWesterosSecondaryVideo();
    expectSetContext();
    createGstGenericPlayerSuccess();

    destroyGstGenericPlayerSuccess();
}

/**
 * Test that a GstGenericPlayer object can be created successfully for a secondary video if height is less than the minimum.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateDestroySecondaryVideoMinHeightSuccess)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;
    expectSetWesterosSecondaryVideo();
    expectSetContext();
    createGstGenericPlayerSuccess();

    destroyGstGenericPlayerSuccess();
}

/**
 * Test that a GstGenericPlayer can be created successfully for a secondary video if no westeros sink.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateDestroySecondaryVideoNoWesterosSuccess)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink"))).WillOnce(Return(nullptr));

    expectSetContext();
    createGstGenericPlayerSuccess();

    destroyGstGenericPlayerSuccess();
}

/**
 * Test that a GstGenericPlayer can be created successfully for a secondary video if westerosink creation fails, but
 * context set successfully.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateWesterossinkFailureSetContextSuccessForSecondaryVideoSuccess)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink")))
        .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_westerosFactory)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_westerosFactory), _))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_westerosFactory)));
    expectSetContext();

    createGstGenericPlayerSuccess();
    destroyGstGenericPlayerSuccess();
}

/**
 * Test that a GstGenericPlayer can be created successfully for a secondary video if failure to set the res-usage flag
 * on a westerossink, but context set successfully.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, SetResUsageFailureSetContextSuccessForSecondaryVideoSuccess)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;
    expectSetContext();

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink")))
        .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_westerosFactory)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_westerosFactory), _))
        .WillOnce(Return(&m_westerosSink));
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("res-usage"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_westerosSink)));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_westerosFactory)));

    createGstGenericPlayerSuccess();
    destroyGstGenericPlayerSuccess();
}

/**
 * Test that a GstGenericPlayer can be created successfully for a secondary video if westerossink is created
 * successfully, but creating context fails
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateWesterossinkSuccessCreateContextFailureForSecondaryVideoSuccess)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;

    expectSetWesterosSecondaryVideo();

    EXPECT_CALL(*m_gstWrapperMock, gstContextNew(StrEq("erm"), false)).WillOnce(Return(nullptr));
    createGstGenericPlayerSuccess();
    destroyGstGenericPlayerSuccess();
}

/**
 * Test that a GstGenericPlayer can be created successfully for a secondary video if westerossink is created
 * successfully, but setting structure fails
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateWesterossinkSuccessCreateStructureFailureForSecondaryVideoSuccess)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;

    expectSetWesterosSecondaryVideo();

    EXPECT_CALL(*m_gstWrapperMock, gstContextNew(StrEq("erm"), false))
        .WillOnce(Return(reinterpret_cast<GstContext *>(&m_dummyContext)));
    EXPECT_CALL(*m_gstWrapperMock, gstContextWritableStructure(reinterpret_cast<GstContext *>(&m_dummyContext)))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstContextUnref(reinterpret_cast<GstContext *>(&m_dummyContext)));
    createGstGenericPlayerSuccess();
    destroyGstGenericPlayerSuccess();
}

/**
 * Test that a GstGenericPlayer creation fails for a secondary video if westerossink creation failsand context creation fails
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateWesterossinkFailsCreateContextFailureForSecondaryVideoFailure)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;
    expectCreatePipeline();
    gstPlayerWillBeDestroyed();
    executeTaskWhenEnqueued();
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink")))
        .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_westerosFactory)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_westerosFactory), _))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_westerosFactory)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_READY))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));

    EXPECT_CALL(*m_gstWrapperMock, gstContextNew(StrEq("erm"), false)).WillOnce(Return(nullptr));
    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  m_rdkGstreamerUtilsWrapperMock, m_gstInitialiserMock,
                                                                  std::move(m_flushWatcher), m_gstSrcFactoryMock,
                                                                  m_timerFactoryMock, std::move(m_taskFactory),
                                                                  std::move(workerThreadFactory),
                                                                  std::move(gstDispatcherThreadFactory),
                                                                  m_gstProtectionMetadataFactoryMock),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstGenericPlayer throws an exception if GstSrcFactory is null.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, GstSrcFactoryNull)
{
    EXPECT_CALL(m_gstInitialiserMock, waitForInitialisation());
    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  m_rdkGstreamerUtilsWrapperMock, m_gstInitialiserMock,
                                                                  std::move(m_flushWatcher), nullptr,
                                                                  m_timerFactoryMock, std::move(m_taskFactory),
                                                                  std::move(workerThreadFactory),
                                                                  std::move(gstDispatcherThreadFactory),
                                                                  m_gstProtectionMetadataFactoryMock),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstGenericPlayer throws an exception if TimerFactory is invalid
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, TimerFactoryFails)
{
    EXPECT_CALL(m_gstInitialiserMock, waitForInitialisation());
    initFactories();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  m_rdkGstreamerUtilsWrapperMock, m_gstInitialiserMock,
                                                                  std::move(m_flushWatcher), m_gstSrcFactoryMock,
                                                                  nullptr, std::move(m_taskFactory),
                                                                  std::move(workerThreadFactory),
                                                                  std::move(gstDispatcherThreadFactory),
                                                                  m_gstProtectionMetadataFactoryMock),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstGenericPlayer throws an exception if GstSrcFactory fails to create GstSrc.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, GstSrcFactoryFails)
{
    EXPECT_CALL(m_gstInitialiserMock, waitForInitialisation());
    EXPECT_CALL(*m_gstSrcFactoryMock, getGstSrc()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  m_rdkGstreamerUtilsWrapperMock, m_gstInitialiserMock,
                                                                  std::move(m_flushWatcher), m_gstSrcFactoryMock,
                                                                  m_timerFactoryMock, std::move(m_taskFactory),
                                                                  std::move(workerThreadFactory),
                                                                  std::move(gstDispatcherThreadFactory),
                                                                  m_gstProtectionMetadataFactoryMock),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstGenericPlayer throws an exception an unknown media type is used.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, UnknownMediaType)
{
    EXPECT_CALL(m_gstInitialiserMock, waitForInitialisation());
    initFactories();
    expectShutdown();
    executeTaskWhenEnqueued();
    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstProtectionMetadataFactoryMock, createProtectionMetadataWrapper(_))
        .WillOnce(Return(ByMove(std::move(m_gstProtectionMetadataWrapper))));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock,
                                                                  MediaType::UNKNOWN, m_videoReq, m_gstWrapperMock,
                                                                  m_glibWrapperMock, m_rdkGstreamerUtilsWrapperMock,
                                                                  m_gstInitialiserMock, std::move(m_flushWatcher),
                                                                  m_gstSrcFactoryMock, m_timerFactoryMock,
                                                                  std::move(m_taskFactory), std::move(workerThreadFactory),
                                                                  std::move(gstDispatcherThreadFactory),
                                                                  m_gstProtectionMetadataFactoryMock),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstGenericPlayer is still created if playsink not found.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, PlaysinkNotFound)
{
    EXPECT_CALL(m_gstInitialiserMock, waitForInitialisation());
    initFactories();

    expectMakePlaybin();
    expectSetFlags();
    expectSetSignalCallbacks();
    expectSetUri();

    expectSetMessageCallback();

    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstProtectionMetadataFactoryMock, createProtectionMetadataWrapper(_))
        .WillOnce(Return(ByMove(std::move(m_gstProtectionMetadataWrapper))));

    EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(_, StrEq("playsink"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_READY))
        .WillOnce(Return(GST_STATE_CHANGE_FAILURE));

    EXPECT_NO_THROW(
        m_gstPlayer =
            std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type, m_videoReq,
                                               m_gstWrapperMock, m_glibWrapperMock, m_rdkGstreamerUtilsWrapperMock,
                                               m_gstInitialiserMock, std::move(m_flushWatcher), m_gstSrcFactoryMock,
                                               m_timerFactoryMock, std::move(m_taskFactory),
                                               std::move(workerThreadFactory), std::move(gstDispatcherThreadFactory),
                                               m_gstProtectionMetadataFactoryMock));
    EXPECT_NE(m_gstPlayer, nullptr);

    executeTaskWhenEnqueued();
    gstPlayerWillBeDestroyed();
}

/**
 * Test that a GstGenericPlayer sets native audio flag from brcmaudiosink.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, SetNativeAudioForBrcmAudioSink)
{
    EXPECT_CALL(m_gstInitialiserMock, waitForInitialisation());
    initFactories();

    expectMakePlaybin();
    expectSetFlagsWithNativeAudio();
    expectSetSignalCallbacks();
    expectSetUri();
    expectCheckPlaySink();
    expectSetMessageCallback();

    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_READY))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstProtectionMetadataFactoryMock, createProtectionMetadataWrapper(_))
        .WillOnce(Return(ByMove(std::move(m_gstProtectionMetadataWrapper))));

    EXPECT_NO_THROW(
        m_gstPlayer =
            std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type, m_videoReq,
                                               m_gstWrapperMock, m_glibWrapperMock, m_rdkGstreamerUtilsWrapperMock,
                                               m_gstInitialiserMock, std::move(m_flushWatcher), m_gstSrcFactoryMock,
                                               m_timerFactoryMock, std::move(m_taskFactory),
                                               std::move(workerThreadFactory), std::move(gstDispatcherThreadFactory),
                                               m_gstProtectionMetadataFactoryMock));
    EXPECT_NE(m_gstPlayer, nullptr);

    executeTaskWhenEnqueued();
    gstPlayerWillBeDestroyed();
}
