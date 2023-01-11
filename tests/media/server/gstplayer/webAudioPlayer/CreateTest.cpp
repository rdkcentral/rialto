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

#include "WebAudioPlayerTaskMock.h"
#include "GstWebAudioPlayerTestCommon.h"
#include "Matchers.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;

class RialtoServerCreateGstWebAudioPlayerTest : public GstWebAudioPlayerTestCommon
{
protected:
    std::unique_ptr<IGstWebAudioPlayer> m_gstPlayer;

    WebAudioPlayerContext m_storedPlayerContext;

    void createGstWebAudioPlayerSuccess()
    {
        gstPlayerWillBeCreated();

        EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_decryptionServiceMock,
                                                                         m_type, m_videoReq, m_gstWrapperMock,
                                                                         m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                         m_timerFactoryMock, std::move(m_taskFactory),
                                                                         std::move(workerThreadFactory),
                                                                         std::move(gstDispatcherThreadFactory),
                                                                         m_gstProtectionMetadataFactoryMock););
        EXPECT_NE(m_gstPlayer, nullptr);
    }

    void destroyGstWebAudioPlayerSuccess()
    {
        gstPlayerWillBeDestroyed();

        m_gstPlayer.reset();
    }

    void setupElementSuccess()
    {
        GstElement element{};
        std::unique_ptr<IPlayerTask> task{std::make_unique<StrictMock<WebAudioPlayerTaskMock>>()};
        EXPECT_CALL(dynamic_cast<StrictMock<WebAudioPlayerTaskMock> &>(*task), execute());
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(&element));
        EXPECT_CALL(m_taskFactoryMock, createSetupElement(_, _, &element))
            .WillOnce(DoAll(SaveArg<0>(&m_storedPlayerContext), Return(ByMove(std::move(task)))));

        triggerSetupElement(&element);
    }
};

/**
 * Test that a GstWebAudioPlayer object can be created successfully for the llama platform.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, CreateDestroyLlamaSuccess)
{
    gstPlayerWillBeCreatedForLlama();

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_gstWrapperMock,
                                                                        m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                        m_timerFactoryMock, std::move(m_taskFactory),
                                                                        std::move(workerThreadFactory),
                                                                        std::move(gstDispatcherThreadFactory)););
    EXPECT_NE(m_gstPlayer, nullptr);

    gstPlayerWillBeDestroyed();
    m_gstPlayer.reset();
}

/**
 * Test that a GstWebAudioPlayer object can be created successfully for the xione platform.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, CreateDestroyXiOneSuccess)
{
    gstPlayerWillBeCreatedForXiOne();

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_gstWrapperMock,
                                                                        m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                        m_timerFactoryMock, std::move(m_taskFactory),
                                                                        std::move(workerThreadFactory),
                                                                        std::move(gstDispatcherThreadFactory)););
    EXPECT_NE(m_gstPlayer, nullptr);

    gstPlayerWillBeDestroyed();
    m_gstPlayer.reset();
}

/**
 * Test that a GstWebAudioPlayer object can be created successfully for the xione platform.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, CreateDestroyXiOneSuccess)
{
    gstPlayerWillBeCreatedForXiOne();

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_gstWrapperMock,
                                                                        m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                        m_timerFactoryMock, std::move(m_taskFactory),
                                                                        std::move(workerThreadFactory),
                                                                        std::move(gstDispatcherThreadFactory)););
    EXPECT_NE(m_gstPlayer, nullptr);

    gstPlayerWillBeDestroyed();
    m_gstPlayer.reset();
}


/**
 * Test that a GstWebAudioPlayer object can be created successfully for the other platforms.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, CreateDestroyGenericPlatformsSuccess)
{
    gstPlayerWillBeCreatedForGenericPlatform();

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_gstWrapperMock,
                                                                        m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                        m_timerFactoryMock, std::move(m_taskFactory),
                                                                        std::move(workerThreadFactory),
                                                                        std::move(gstDispatcherThreadFactory)););
    EXPECT_NE(m_gstPlayer, nullptr);

    gstPlayerWillBeDestroyed();
    m_gstPlayer.reset();
}