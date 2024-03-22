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
    willConfigureSocket();
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
