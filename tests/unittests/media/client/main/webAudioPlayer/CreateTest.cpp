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

#include "ClientControllerMock.h"
#include "IWebAudioPlayerIpcClient.h"
#include "MediaFrameWriterFactoryMock.h"
#include "WebAudioPlayer.h"
#include "WebAudioPlayerClientMock.h"
#include "WebAudioPlayerIpcFactoryMock.h"
#include "WebAudioPlayerIpcMock.h"
#include "WebAudioPlayerProxy.h"
#include "WebAudioPlayerTestBase.h"
#include "WebAudioUtil.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::DoubleEq;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{
MATCHER_P(webAudioConfigMatcher, config, "")
{
    std::shared_ptr<const firebolt::rialto::WebAudioConfig> argConfig = arg.lock();
    return argConfig && argConfig->pcm == config->pcm;
}
} // namespace

class RialtoClientCreateWebAudioPlayerTest : public WebAudioPlayerTestBase
{
public:
    void prepareForWebAudioPlayerConstructor(std::unique_ptr<StrictMock<WebAudioPlayerIpcMock>> &webAudioPlayerIpcMock)
    {
        // Save a raw pointer to the unique object for use when testing mocks
        // Object shall be freed by the holder of the unique ptr on destruction
        m_webAudioPlayerIpcMock = webAudioPlayerIpcMock.get();

        EXPECT_CALL(*m_webAudioPlayerIpcFactoryMock,
                    createWebAudioPlayerIpc(_, m_audioMimeType, m_priority, webAudioConfigMatcher(m_config), _))
            .WillOnce(Return(ByMove(std::move(webAudioPlayerIpcMock))));
    }
};

/**
 * Test that a WebAudioPlayer object can be created successfully.
 */
TEST_F(RialtoClientCreateWebAudioPlayerTest, Create)
{
    std::unique_ptr<StrictMock<WebAudioPlayerIpcMock>> webAudioPlayerIpcMock =
        std::make_unique<StrictMock<WebAudioPlayerIpcMock>>();

    prepareForWebAudioPlayerConstructor(webAudioPlayerIpcMock);

    EXPECT_NO_THROW(m_webAudioPlayer = std::make_unique<WebAudioPlayer>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                        m_priority, m_config,
                                                                        m_webAudioPlayerIpcFactoryMock,
                                                                        *m_clientControllerMock));

    EXPECT_NE(m_webAudioPlayer, nullptr);
}

/**
 * Test the WebAudioPlayer proxy
 */
TEST_F(RialtoClientCreateWebAudioPlayerTest, CreateWebAudioPlayerProxy)
{
    std::unique_ptr<StrictMock<WebAudioPlayerIpcMock>> webAudioPlayerIpcMock =
        std::make_unique<StrictMock<WebAudioPlayerIpcMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_webAudioPlayerIpcMock = webAudioPlayerIpcMock.get();

    EXPECT_CALL(*m_webAudioPlayerIpcFactoryMock,
                createWebAudioPlayerIpc(_, m_audioMimeType, m_priority, webAudioConfigMatcher(m_config), _))
        .WillOnce(Return(ByMove(std::move(webAudioPlayerIpcMock))));

    EXPECT_CALL(*m_clientControllerMock, registerClient(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(ApplicationState::RUNNING), Return(true)));
    EXPECT_CALL(*m_clientControllerMock, unregisterClient(_)).WillOnce(Return(true));

    std::shared_ptr<IWebAudioPlayerAndIControlClient> webAudioPlayer;
    webAudioPlayer = std::make_shared<WebAudioPlayer>(m_webAudioPlayerClientMock, m_audioMimeType, m_priority, m_config,
                                                      m_webAudioPlayerIpcFactoryMock, *m_clientControllerMock);
    EXPECT_NE(webAudioPlayer, nullptr);

    std::shared_ptr<WebAudioPlayerProxy> proxy;
    EXPECT_NO_THROW(proxy = std::make_shared<WebAudioPlayerProxy>(webAudioPlayer, *m_clientControllerMock));
    EXPECT_NE(proxy, nullptr);
}

/**
 * Test the factory
 */
TEST_F(RialtoClientCreateWebAudioPlayerTest, FactoryCreatesObject)
{
    std::shared_ptr<firebolt::rialto::WebAudioPlayerFactory> factory =
        std::dynamic_pointer_cast<firebolt::rialto::WebAudioPlayerFactory>(
            firebolt::rialto::IWebAudioPlayerFactory::createFactory());
    EXPECT_NE(factory, nullptr);
    EXPECT_CALL(*m_clientControllerMock, registerClient(_, _)).WillOnce(Return(true));

    std::unique_ptr<StrictMock<WebAudioPlayerIpcMock>> webAudioPlayerIpcMock =
        std::make_unique<StrictMock<WebAudioPlayerIpcMock>>();

    prepareForWebAudioPlayerConstructor(webAudioPlayerIpcMock);

    std::unique_ptr<IWebAudioPlayer> webAudioPlayer;
    EXPECT_NO_THROW(webAudioPlayer = factory->createWebAudioPlayer(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                   m_priority, m_config, m_webAudioPlayerIpcFactoryMock,
                                                                   m_clientControllerMock));
    EXPECT_NE(webAudioPlayer, nullptr);

    // Unregister client on destroy
    EXPECT_CALL(*m_clientControllerMock, unregisterClient(_)).WillOnce(Return(true));
}

/**
 * Test factory returns a nullptr if the creation of the object fails.
 */
TEST_F(RialtoClientCreateWebAudioPlayerTest, FactoryFailsToCreateObject)
{
    std::shared_ptr<firebolt::rialto::WebAudioPlayerFactory> factory =
        std::dynamic_pointer_cast<firebolt::rialto::WebAudioPlayerFactory>(
            firebolt::rialto::IWebAudioPlayerFactory::createFactory());
    EXPECT_NE(factory, nullptr);

    EXPECT_CALL(*m_webAudioPlayerIpcFactoryMock, createWebAudioPlayerIpc(_, _, _, _, _)).WillOnce(Return(nullptr));

    std::unique_ptr<IWebAudioPlayer> webAudioPlayer;
    EXPECT_NO_THROW(webAudioPlayer = factory->createWebAudioPlayer(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                   m_priority, m_config, m_webAudioPlayerIpcFactoryMock,
                                                                   m_clientControllerMock));
    EXPECT_EQ(webAudioPlayer, nullptr);
}

/**
 * Test that a WebAudioPlayer object throws an exeption if failure occurs during construction.
 * In this case, createWebAudioPlayerIpc fails, returning a nullptr.
 */
TEST_F(RialtoClientCreateWebAudioPlayerTest, CreateWebAudioPlayerIpcFailure)
{
    std::unique_ptr<IWebAudioPlayer> webAudioPlayer;

    EXPECT_CALL(*m_webAudioPlayerIpcFactoryMock, createWebAudioPlayerIpc(_, _, _, _, _)).WillOnce(Return(nullptr));

    EXPECT_THROW(webAudioPlayer = std::make_unique<WebAudioPlayer>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                   m_priority, m_config, m_webAudioPlayerIpcFactoryMock,
                                                                   *m_clientControllerMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer object throws an exeption if failure occurs during construction.
 * In this case, createWebAudioPlayerIpc fails, returning a nullptr.
 */
TEST_F(RialtoClientCreateWebAudioPlayerTest, CreateWebAudioPlayerFailure)
{
    EXPECT_CALL(*m_webAudioPlayerIpcFactoryMock,
                createWebAudioPlayerIpc(_, m_audioMimeType, m_priority, webAudioConfigMatcher(m_config), _))
        .WillOnce(Return(nullptr));

    std::shared_ptr<IWebAudioPlayerAndIControlClient> webAudioPlayer;
    EXPECT_THROW(webAudioPlayer = std::make_shared<WebAudioPlayer>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                   m_priority, m_config, m_webAudioPlayerIpcFactoryMock,
                                                                   *m_clientControllerMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayer proxy throws an exeption if it fails to
 * register the webAudioPlayer
 */
TEST_F(RialtoClientCreateWebAudioPlayerTest, CreateWebAudioPlayerProxyRegisterFailure)
{
    std::unique_ptr<StrictMock<WebAudioPlayerIpcMock>> webAudioPlayerIpcMock =
        std::make_unique<StrictMock<WebAudioPlayerIpcMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_webAudioPlayerIpcMock = webAudioPlayerIpcMock.get();

    EXPECT_CALL(*m_webAudioPlayerIpcFactoryMock,
                createWebAudioPlayerIpc(_, m_audioMimeType, m_priority, webAudioConfigMatcher(m_config), _))
        .WillOnce(Return(ByMove(std::move(webAudioPlayerIpcMock))));

    EXPECT_CALL(*m_clientControllerMock, registerClient(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(ApplicationState::RUNNING), Return(false)));

    std::shared_ptr<IWebAudioPlayerAndIControlClient> webAudioPlayer;
    webAudioPlayer = std::make_unique<WebAudioPlayer>(m_webAudioPlayerClientMock, m_audioMimeType, m_priority, m_config,
                                                      m_webAudioPlayerIpcFactoryMock, *m_clientControllerMock);
    EXPECT_NE(webAudioPlayer, nullptr);

    std::shared_ptr<WebAudioPlayerProxy> proxy;
    EXPECT_THROW(proxy = std::make_shared<WebAudioPlayerProxy>(webAudioPlayer, *m_clientControllerMock),
                 std::runtime_error);
}
