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

#include "MediaKeysTestBase.h"

class RialtoServerCreateMediaKeysTest : public MediaKeysTestBase
{
};

/**
 * Test that a MediaKeysServerInternal object can be created successfully.
 */
TEST_F(RialtoServerCreateMediaKeysTest, Create)
{
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kWidevineKeySystem))
        .WillOnce(Return(ByMove(std::move(m_ocdmSystem))));
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_NO_THROW(m_mediaKeys = std::make_unique<MediaKeysServerInternal>(kWidevineKeySystem, m_mainThreadFactoryMock,
                                                                            m_ocdmSystemFactoryMock,
                                                                            m_mediaKeySessionFactoryMock));
    EXPECT_NE(m_mediaKeys, nullptr);

    EXPECT_CALL(*m_mainThreadMock, unregisterClient(m_kMainThreadClientId));
    // Objects are destroyed on the main thread
    mainThreadWillEnqueueTaskAndWait();
}

/**
 * Test the external factory (this code is designed to fail)
 */
TEST_F(RialtoServerCreateMediaKeysTest, ExternalFactoryFail)
{
    std::shared_ptr<firebolt::rialto::IMediaKeysFactory> factory = firebolt::rialto::IMediaKeysFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    // The following is expected to return null, and show an error log
    EXPECT_EQ(factory->createMediaKeys(kWidevineKeySystem), nullptr);
}

/**
 * Test the internal factory
 */
TEST_F(RialtoServerCreateMediaKeysTest, InternalFactoryFails)
{
    std::shared_ptr<IMediaKeysServerInternalFactory> factory =
        firebolt::rialto::server::IMediaKeysServerInternalFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    // We expect the following to fail
    // The test harness uses a stub for  IOcdmSystemFactory::createFactory()  which is defined in
    //    "./tests/media/server/stubs/wrappers/OcdmSystemFactory.cpp" and returns null
    EXPECT_EQ(factory->createMediaKeysServerInternal(kWidevineKeySystem), nullptr);
}

/**
 * Test that a MediaKeys object throws an exeption if failure occurs during construction.
 * In this case, getMainThread fails, returning a nullptr.
 */
TEST_F(RialtoServerCreateMediaKeysTest, GetMainThreadFailure)
{
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_mediaKeys = std::make_unique<MediaKeysServerInternal>(kWidevineKeySystem, m_mainThreadFactoryMock,
                                                                         m_ocdmSystemFactoryMock,
                                                                         m_mediaKeySessionFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaKeysServerInternal object throws an exeption if failure occurs during construction.
 * In this case, createOcdmSystem fails, returning a nullptr.
 */
TEST_F(RialtoServerCreateMediaKeysTest, CreateOcdmSystemFailure)
{
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kWidevineKeySystem)).WillOnce(Return(ByMove(std::move(nullptr))));
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_THROW(m_mediaKeys = std::make_unique<MediaKeysServerInternal>(kWidevineKeySystem, m_mainThreadFactoryMock,
                                                                         m_ocdmSystemFactoryMock,
                                                                         m_mediaKeySessionFactoryMock),
                 std::runtime_error);
}
