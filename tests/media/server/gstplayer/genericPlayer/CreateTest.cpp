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

#include "GstGenericPlayerTestCommon.h"
#include "Matchers.h"
#include "PlayerTaskMock.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;

class RialtoServerCreateGstGenericPlayerTest : public GstGenericPlayerTestCommon
{
protected:
    std::unique_ptr<IGstGenericPlayer> m_gstPlayer;

    MediaType m_type{MediaType::MSE};
    VideoRequirements m_videoReq = {kMinPrimaryVideoWidth, kMinPrimaryVideoHeight};
    GenericPlayerContext m_storedPlayerContext;
    GstObject m_westerousFactory{}; // GstElementFactory is an opaque data structure
    GstElement m_westerousSink{};
    GParamSpec m_rectangleSpec{};

    void expectCreatePipeline()
    {
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

        EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock,
                                                                         m_type, m_videoReq, m_gstWrapperMock,
                                                                         m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                         m_timerFactoryMock, std::move(m_taskFactory),
                                                                         std::move(workerThreadFactory),
                                                                         std::move(gstDispatcherThreadFactory),
                                                                         m_gstProtectionMetadataFactoryMock));
        EXPECT_NE(m_gstPlayer, nullptr);
    }

    void destroyGstGenericPlayerSuccess()
    {
        gstPlayerWillBeDestroyed();

        m_gstPlayer.reset();
    }

    void expectSetSecondaryVideo()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("westerossink")))
            .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_westerousFactory)));
        EXPECT_CALL(*m_gstWrapperMock,
                    gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_westerousFactory), _))
            .WillOnce(Return(&m_westerousSink));
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, CharStrMatcher("res-usage")))
            .WillOnce(Return(&m_rectangleSpec));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, CharStrMatcher("res-usage")));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(_, CharStrMatcher("video-sink")));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_westerousFactory)));
    }
};

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
    expectSetSecondaryVideo();
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
    expectSetSecondaryVideo();
    createGstGenericPlayerSuccess();

    destroyGstGenericPlayerSuccess();
}

/**
 * Test that a GstGenericPlayer can be created successfully for a secondary video if no westerous sink.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateDestroySecondaryVideoNoWesterousSuccess)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("westerossink"))).WillOnce(Return(nullptr));

    createGstGenericPlayerSuccess();

    destroyGstGenericPlayerSuccess();
}

/**
 * Test that a GstGenericPlayer object throws an exception if failure to create a westeroussink.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, CreateWesterossinkFailureForSecondaryVideo)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;
    expectCreatePipeline();
    gstPlayerWillBeDestroyed();
    executeTaskWhenEnqueued();

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("westerossink")))
        .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_westerousFactory)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_westerousFactory), _))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_westerousFactory)));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  m_gstSrcFactoryMock, m_timerFactoryMock,
                                                                  std::move(m_taskFactory), std::move(workerThreadFactory),
                                                                  std::move(gstDispatcherThreadFactory),
                                                                  m_gstProtectionMetadataFactoryMock),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstGenericPlayer object throws an exception if failure to set the res-usage flag on a westerossink.
 */
TEST_F(RialtoServerCreateGstGenericPlayerTest, SetResUsageFailureForSecondaryVideo)
{
    // Height < minimum
    m_videoReq.maxWidth = kMinPrimaryVideoWidth;
    m_videoReq.maxHeight = kMinPrimaryVideoHeight - 1;
    expectCreatePipeline();
    gstPlayerWillBeDestroyed();
    executeTaskWhenEnqueued();

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("westerossink")))
        .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_westerousFactory)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(reinterpret_cast<GstElementFactory *>(&m_westerousFactory), _))
        .WillOnce(Return(&m_westerousSink));
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, CharStrMatcher("res-usage"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_westerousSink)));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_westerousFactory)));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  m_gstSrcFactoryMock, m_timerFactoryMock,
                                                                  std::move(m_taskFactory), std::move(workerThreadFactory),
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
    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  nullptr, m_timerFactoryMock, std::move(m_taskFactory),
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
    initFactories();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  m_gstSrcFactoryMock, nullptr, std::move(m_taskFactory),
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
    EXPECT_CALL(*m_gstSrcFactoryMock, getGstSrc()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, m_type,
                                                                  m_videoReq, m_gstWrapperMock, m_glibWrapperMock,
                                                                  m_gstSrcFactoryMock, m_timerFactoryMock,
                                                                  std::move(m_taskFactory), std::move(workerThreadFactory),
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
    initFactories();
    expectShutdown();
    executeTaskWhenEnqueued();
    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstProtectionMetadataFactoryMock, createProtectionMetadataWrapper(_))
        .WillOnce(Return(ByMove(std::move(m_gstProtectionMetadataWrapper))));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock,
                                                                  MediaType::UNKNOWN, m_videoReq, m_gstWrapperMock,
                                                                  m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                  m_timerFactoryMock, std::move(m_taskFactory),
                                                                  std::move(workerThreadFactory),
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

    EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(_, CharStrMatcher("playsink"))).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock,
                                                                     m_type, m_videoReq, m_gstWrapperMock,
                                                                     m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                     m_timerFactoryMock, std::move(m_taskFactory),
                                                                     std::move(workerThreadFactory),
                                                                     std::move(gstDispatcherThreadFactory),
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
    initFactories();

    expectMakePlaybin();
    expectSetFlagsWithNativeAudio();
    expectSetSignalCallbacks();
    expectSetUri();
    expectCheckPlaySink();
    expectSetMessageCallback();

    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstProtectionMetadataFactoryMock, createProtectionMetadataWrapper(_))
        .WillOnce(Return(ByMove(std::move(m_gstProtectionMetadataWrapper))));

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock,
                                                                     m_type, m_videoReq, m_gstWrapperMock,
                                                                     m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                     m_timerFactoryMock, std::move(m_taskFactory),
                                                                     std::move(workerThreadFactory),
                                                                     std::move(gstDispatcherThreadFactory),
                                                                     m_gstProtectionMetadataFactoryMock));
    EXPECT_NE(m_gstPlayer, nullptr);

    executeTaskWhenEnqueued();
    gstPlayerWillBeDestroyed();
}
