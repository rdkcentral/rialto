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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "MessageBuilders.h"
#include "OcdmSessionMock.h"
#include "RialtoServerComponentTest.h"

using testing::_;
using testing::ByMove;
using testing::Return;
using testing::StrictMock;

namespace firebolt::rialto::server::ct
{
class MediaKeysTest : public RialtoServerComponentTest
{
public:
    MediaKeysTest()
    {
        willConfigureSocket();
        configureSutInActiveState();
        connectClient();
    }
    ~MediaKeysTest() override = default;

    void createMediaKeys()
    {
        // Use matchResponse to store media keys handle
        auto request{createCreateMediaKeysRequest()};
        ConfigureAction<CreateMediaKeys>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const ::firebolt::rialto::CreateMediaKeysResponse &resp)
                           { m_mediaKeysHandle = resp.media_keys_handle(); });
    }

    void createSession()
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

    void sessionWillBeCreated()
    {
        EXPECT_CALL(*m_ocdmSystemMock, createSession(_)).WillOnce(Return(ByMove(std::move(m_ocdmSession))));
    }

    int m_mediaKeysHandle{-1};
    int m_mediaKeySessionId{-1};
    std::unique_ptr<StrictMock<wrappers::OcdmSessionMock>> m_ocdmSession{
        std::make_unique<StrictMock<wrappers::OcdmSessionMock>>()};
    StrictMock<wrappers::OcdmSessionMock> &m_ocdmSessionMock{*m_ocdmSession};
};

TEST_F(MediaKeysTest, shouldCreateMediaKeys)
{
    createMediaKeys();
}

TEST_F(MediaKeysTest, shouldFailToCreateSessionWhenMksIdIsWrong)
{
    createMediaKeys();
    auto request{createCreateKeySessionRequest(m_mediaKeysHandle + 1)};
    ConfigureAction<CreateKeySession>(m_clientStub)
        .send(request)
        .expectSuccess() // sick!
        .matchResponse([&](const firebolt::rialto::CreateKeySessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::FAIL); });
}

TEST_F(MediaKeysTest, shouldCreateSession)
{
    createMediaKeys();
    sessionWillBeCreated();
    createSession();
}
} // namespace firebolt::rialto::server::ct
