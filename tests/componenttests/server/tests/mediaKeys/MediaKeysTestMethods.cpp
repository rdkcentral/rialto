/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include <utility>

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ExpectMessage.h"
#include "Matchers.h"
#include "MediaKeysTestMethods.h"

using testing::_;
using testing::ByMove;
using testing::Return;
using testing::StrictMock;

namespace firebolt::rialto::server::ct
{
MediaKeysTestMethods::MediaKeysTestMethods()
{
    configureSutInActiveState();
    connectClient();
}

void MediaKeysTestMethods::createMediaKeys(const ::firebolt::rialto::CreateMediaKeysRequest &request)
{
    // Use matchResponse to store media keys handle
    ConfigureAction<CreateMediaKeys>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const ::firebolt::rialto::CreateMediaKeysResponse &resp)
                       { m_mediaKeysHandle = resp.media_keys_handle(); });
}

void MediaKeysTestMethods::createMediaKeysWidevine()
{
    createMediaKeys(createCreateMediaKeysRequestWidevine());
}

void MediaKeysTestMethods::createMediaKeysNetflix()
{
    createMediaKeys(createCreateMediaKeysRequestNetflix());
}

void MediaKeysTestMethods::createKeySession()
{
    // Use matchResponse to store media key session id
    auto request{createCreateKeySessionRequest(m_mediaKeysHandle)};
    ConfigureAction<CreateKeySession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::CreateKeySessionResponse &resp)
            {
                m_mediaKeySessionId = resp.key_session_id();
                EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
            });
}

void MediaKeysTestMethods::ocdmSessionWillBeCreated()
{
    EXPECT_CALL(*m_ocdmSystemMock, createSession(_))
        .WillOnce(testing::Invoke(
            [&](firebolt::rialto::wrappers::IOcdmSessionClient *client)
                -> std::unique_ptr<firebolt::rialto::wrappers::IOcdmSession>
            {
                m_ocdmSessionClient = client;
                return std::move(m_ocdmSession);
            }));
}

void MediaKeysTestMethods::willGenerateRequestPlayready()
{
    EXPECT_CALL(m_ocdmSessionMock, constructSession(KeySessionType::TEMPORARY, InitDataType::CENC, _, m_kInitData.size()))
        .WillOnce(testing::Invoke(
            [&](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[],
                uint32_t initDataSize) -> MediaKeyErrorStatus
            {
                for (uint32_t i = 0; i < initDataSize; ++i)
                {
                    EXPECT_EQ(initData[i], m_kInitData[i]);
                }

                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_CALL(m_ocdmSessionMock, getChallengeData(false, _, _))
        .WillOnce(testing::Invoke(
            [&](bool isLDL, const uint8_t *challenge, uint32_t *challengeSize) -> MediaKeyErrorStatus
            {
                // This first call asks for the size of the data
                EXPECT_EQ(challenge, nullptr);
                *challengeSize = m_kLicenseRequestMessage.size();
                return MediaKeyErrorStatus::OK;
            }))
        .WillOnce(testing::Invoke(
            [&](bool isLDL, uint8_t *challenge, const uint32_t *challengeSize) -> MediaKeyErrorStatus
            {
                // This second call asks for the data
                EXPECT_EQ(*challengeSize, m_kLicenseRequestMessage.size());
                for (size_t i = 0; i < m_kLicenseRequestMessage.size(); ++i)
                {
                    challenge[i] = m_kLicenseRequestMessage[i];
                }
                return MediaKeyErrorStatus::OK;
            }));
}

void MediaKeysTestMethods::generateRequestPlayready()
{
    constexpr bool kUseExtendedInterface{true};
    auto request{createGenerateRequestRequest(m_mediaKeysHandle, m_mediaKeySessionId, m_kInitData, kUseExtendedInterface)};

    ExpectMessage<::firebolt::rialto::LicenseRequestEvent> expectedMessage(m_clientStub);

    ConfigureAction<GenerateRequest>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GenerateRequestResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });

    auto message = expectedMessage.getMessage();
    ASSERT_TRUE(message);
    ASSERT_EQ(message->media_keys_handle(), m_mediaKeysHandle);
    ASSERT_EQ(message->key_session_id(), m_mediaKeySessionId);
    EXPECT_EQ(message->url(), "");
    const unsigned int kMax = message->license_request_message_size();
    ASSERT_EQ(kMax, m_kLicenseRequestMessage.size());
    for (unsigned int i = 0; i < kMax; ++i)
    {
        ASSERT_EQ(message->license_request_message(i), m_kLicenseRequestMessage[i]);
    }
}

void MediaKeysTestMethods::willUpdateSessionNetflix()
{
    EXPECT_CALL(m_ocdmSessionMock, storeLicenseData(_, m_kUpdateSessionNetflixResponse.size()))
        .WillOnce(testing::Invoke(
            [&](const uint8_t challenge[], uint32_t challengeSize) -> MediaKeyErrorStatus
            {
                for (uint32_t i = 0; i < challengeSize; ++i)
                {
                    EXPECT_EQ(challenge[i], m_kUpdateSessionNetflixResponse[i]);
                }
                return MediaKeyErrorStatus::OK;
            }));
}

void MediaKeysTestMethods::updateSessionNetflix()
{
    auto request = createUpdateSessionRequest(m_mediaKeysHandle, m_mediaKeySessionId, m_kUpdateSessionNetflixResponse);
    ConfigureAction<UpdateSession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::UpdateSessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void MediaKeysTestMethods::willCloseKeySessionPlayready()
{
    EXPECT_CALL(m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(m_ocdmSessionMock, cleanDecryptContext()).WillOnce(Return(MediaKeyErrorStatus::OK));
}

void MediaKeysTestMethods::closeKeySessionPlayready()
{
    auto request{createCloseKeySessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

    ConfigureAction<CloseKeySession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::CloseKeySessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void MediaKeysTestMethods::willTeardown()
{
    // For teardown...
    EXPECT_CALL(m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::OK));
}

void MediaKeysTestMethods::willRelease()
{
    EXPECT_CALL(m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));
}

} // namespace firebolt::rialto::server::ct
