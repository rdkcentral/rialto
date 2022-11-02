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
      m_ocdmSystem{std::make_unique<StrictMock<OcdmSystemMock>>()}, m_ocdmSystemMock{m_ocdmSystem.get()}
{
}

MediaKeysTestBase::~MediaKeysTestBase() {}

void MediaKeysTestBase::createMediaKeys(std::string keySystem)
{
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(keySystem)).WillOnce(Return(ByMove(std::move(m_ocdmSystem))));

    EXPECT_NO_THROW(m_mediaKeys = std::make_unique<MediaKeysServerInternal>(keySystem, m_ocdmSystemFactoryMock,
                                                                            m_mediaKeySessionFactoryMock));
    EXPECT_NE(m_mediaKeys, nullptr);
}

void MediaKeysTestBase::createKeySession(std::string keySystem)
{
    EXPECT_CALL(*m_mediaKeySessionFactoryMock, createMediaKeySession(keySystem, _, _, m_keySessionType, _, m_isLDL))
        .WillOnce(Return(ByMove(std::move(m_mediaKeySession))));

    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeys->createKeySession(m_keySessionType, m_mediaKeysClientMock, m_isLDL, m_keySessionId));
}
