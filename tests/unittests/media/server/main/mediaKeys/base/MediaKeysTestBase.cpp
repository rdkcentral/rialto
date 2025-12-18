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
#include <memory>
#include <string>
#include <utility>

MediaKeysTestBase::MediaKeysTestBase()
    : m_mediaKeysClientMock{std::make_shared<StrictMock<MediaKeysClientMock>>()},
      m_mediaKeySessionFactoryMock{std::make_shared<StrictMock<MediaKeySessionFactoryMock>>()},
      m_mediaKeySession{std::make_unique<StrictMock<MediaKeySessionMock>>()},
      m_mediaKeySessionMock{m_mediaKeySession.get()},
      m_ocdmSystemFactoryMock{std::make_shared<StrictMock<OcdmSystemFactoryMock>>()},
      m_ocdmSystem{std::make_shared<StrictMock<OcdmSystemMock>>()}, m_ocdmSystemMock{m_ocdmSystem.get()},
      m_mainThreadFactoryMock{std::make_shared<StrictMock<MainThreadFactoryMock>>()},
      m_mainThreadMock{std::make_shared<StrictMock<MainThreadMock>>()}
{
}

MediaKeysTestBase::~MediaKeysTestBase() {}

void MediaKeysTestBase::createMediaKeys(std::string keySystem)
{
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(keySystem)).WillOnce(Return(ByMove(std::move(m_ocdmSystem))));
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_NO_THROW(m_mediaKeys = std::make_unique<MediaKeysServerInternal>(keySystem, m_mainThreadFactoryMock,
                                                                            m_ocdmSystemFactoryMock,
                                                                            m_mediaKeySessionFactoryMock));
    EXPECT_NE(m_mediaKeys, nullptr);
}

void MediaKeysTestBase::destroyMediaKeys()
{
    EXPECT_CALL(*m_mainThreadMock, unregisterClient(m_kMainThreadClientId));
    // Objects are destroyed on the main thread
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys.reset();
}

void MediaKeysTestBase::createKeySession(std::string keySystem)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionFactoryMock, createMediaKeySession(keySystem, _, _, m_keySessionType, _))
        .WillOnce(Return(ByMove(std::move(m_mediaKeySession))));

    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeys->createKeySession(m_keySessionType, m_mediaKeysClientMock, m_kKeySessionId));
}

void MediaKeysTestBase::mainThreadWillEnqueueTaskAndWait()
{
    EXPECT_CALL(*m_mainThreadMock, enqueueTaskAndWait(m_kMainThreadClientId, _))
        .WillOnce(Invoke([](uint32_t clientId, firebolt::rialto::server::IMainThread::Task task) { task(); }))
        .RetiresOnSaturation();
}

void MediaKeysTestBase::mainThreadWillEnqueueTaskAndWaitMultiple(int numberOfTimes)
{
    EXPECT_CALL(*m_mainThreadMock, enqueueTaskAndWait(m_kMainThreadClientId, _))
        .Times(numberOfTimes)
        .WillRepeatedly(Invoke([](uint32_t clientId, firebolt::rialto::server::IMainThread::Task task) { task(); }));
}
