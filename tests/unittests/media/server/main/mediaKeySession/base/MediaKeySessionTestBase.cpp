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

#include "MediaKeySessionTestBase.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

MediaKeySessionTestBase::MediaKeySessionTestBase()
    : m_mediaKeysClientMock{std::make_shared<StrictMock<MediaKeysClientMock>>()},
      m_ocdmSystemMock{std::make_shared<StrictMock<OcdmSystemMock>>()},
      m_ocdmSession{std::make_unique<StrictMock<OcdmSessionMock>>()}, m_ocdmSessionMock{m_ocdmSession.get()},
      m_mainThreadFactoryMock{std::make_shared<StrictMock<MainThreadFactoryMock>>()},
      m_mainThreadMock{std::make_shared<StrictMock<MainThreadMock>>()}
{
}

MediaKeySessionTestBase::~MediaKeySessionTestBase() {}

void MediaKeySessionTestBase::createKeySession(const std::string &keySystem)
{
    EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
    EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(m_kMainThreadClientId));
    EXPECT_CALL(*m_ocdmSystemMock, createSession(_)).WillOnce(Return(ByMove(std::move(m_ocdmSession))));

    EXPECT_NO_THROW(m_mediaKeySession = std::make_unique<MediaKeySession>(keySystem, m_kKeySessionId, *m_ocdmSystemMock,
                                                                          m_keySessionType, m_mediaKeysClientMock,
                                                                          m_mainThreadFactoryMock));
    EXPECT_NE(m_mediaKeySession, nullptr);
}

void MediaKeySessionTestBase::destroyKeySession()
{
    EXPECT_CALL(*m_mainThreadMock, unregisterClient(m_kMainThreadClientId));
}

void MediaKeySessionTestBase::expectCloseKeySession(const std::string &keySystem)
{
    if (keySystem.find("playready") != std::string::npos)
    {
        EXPECT_CALL(*m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::OK));
        EXPECT_CALL(*m_ocdmSessionMock, cleanDecryptContext()).WillOnce(Return(MediaKeyErrorStatus::OK));
    }
    else
    {
        EXPECT_CALL(*m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::OK));
    }

    EXPECT_CALL(*m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));
}

void MediaKeySessionTestBase::generateRequest()
{
    InitDataType m_initDataType = InitDataType::CENC;
    std::vector<uint8_t> m_initData{1, 2, 3};

    EXPECT_CALL(*m_ocdmSessionMock, constructSession(m_keySessionType, m_initDataType, &m_initData[0], m_initData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_initDataType, m_initData, m_kLdlState));
}

void MediaKeySessionTestBase::generateRequestPlayready()
{
    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptrMatcher(), _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallenge.size()), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, notNullptrMatcher(), _))
        .WillOnce(DoAll(memcpyChallenge(m_kChallenge), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(m_kKeySessionId, m_kChallenge, _));

    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData,
                                                 firebolt::rialto::LimitedDurationLicense::DISABLED));
}

void MediaKeySessionTestBase::generateRequestPlayreadyWithTwoCalls()
{
    EXPECT_CALL(*m_ocdmSessionMock,
                constructSession(m_keySessionType, m_kInitDataType, &m_kInitData[0], m_kInitData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData,
                                                 firebolt::rialto::LimitedDurationLicense::NOT_SPECIFIED));
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, nullptrMatcher(), _))
        .WillOnce(DoAll(SetArgPointee<2>(m_kChallenge.size()), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_ocdmSessionMock, getChallengeData(m_isLDL, notNullptrMatcher(), _))
        .WillOnce(DoAll(memcpyChallenge(m_kChallenge), Return(MediaKeyErrorStatus::OK)));
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(m_kKeySessionId, m_kChallenge, _));

    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeySession->generateRequest(m_kInitDataType, m_kInitData,
                                                 firebolt::rialto::LimitedDurationLicense::DISABLED));
}

void MediaKeySessionTestBase::mainThreadWillEnqueueTask()
{
    EXPECT_CALL(*m_mainThreadMock, enqueueTask(m_kMainThreadClientId, _))
        .WillOnce(Invoke([](uint32_t clientId, firebolt::rialto::server::IMainThread::Task task) { task(); }))
        .RetiresOnSaturation();
}
