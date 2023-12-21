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
        EXPECT_CALL(*m_ocdmSystemMock, createSession(_))
            .WillOnce(testing::Invoke(
                [&](firebolt::rialto::wrappers::IOcdmSessionClient *client)
                    -> std::unique_ptr<firebolt::rialto::wrappers::IOcdmSession>
                {
                    m_client = client;
                    return std::move(m_ocdmSession);
                }));
    }

    void generateRequest()
    {
        auto request{createGenerateRequestRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

        bool ok{false};

        // The following should match the details within the message "request"
        EXPECT_CALL(m_ocdmSessionMock, constructSession(KeySessionType::TEMPORARY, InitDataType::CENC, _, 3))
            .WillOnce(testing::Invoke(
                [&](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[],
                    uint32_t initDataSize) -> MediaKeyErrorStatus
                {
                    ok = (initData[0] == 1 && initData[1] == 2 && initData[2] == 3);
                    return MediaKeyErrorStatus::OK;
                }));

        ConfigureAction<GenerateRequest>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const firebolt::rialto::GenerateRequestResponse &resp)
                           { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });

        ASSERT_EQ(ok, true);

        const std::string kUrl{"http://fictionalUrlForTest"};
        const std::vector<unsigned char> kLicenseRequestMessage{'d', 'z', 'f'};
        std::condition_variable myCondVar;

        bool callFlag{false};
        std::unique_lock<std::mutex> lock1(m_mutex);

        std::function<void(const std::shared_ptr<::firebolt::rialto::LicenseRequestEvent> &message)> handler{
            [&](const std::shared_ptr<LicenseRequestEvent> &message)
            {
                std::unique_lock<std::mutex> lock2(m_mutex);

                ASSERT_EQ(message->media_keys_handle(), m_mediaKeysHandle);
                ASSERT_EQ(message->key_session_id(), m_mediaKeySessionId);
                EXPECT_TRUE(message->url() == kUrl);
                unsigned int max = message->license_request_message_size();
                ASSERT_EQ(max, kLicenseRequestMessage.size());
                for (unsigned int i = 0; i < max; ++i)
                {
                    ASSERT_EQ(message->license_request_message(i), kLicenseRequestMessage[i]);
                }

                callFlag = true;
                myCondVar.notify_all();
            }};
        m_clientStub.getIpcChannel()->subscribe(handler);

        m_client->onProcessChallenge(kUrl.c_str(), &kLicenseRequestMessage[0], kLicenseRequestMessage.size());

        myCondVar.wait_for(lock1, std::chrono::milliseconds{110});
        EXPECT_TRUE(callFlag);

        // For teardown...
        EXPECT_CALL(m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::OK));
        EXPECT_CALL(m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));
    }

    int m_mediaKeysHandle{-1};
    int m_mediaKeySessionId{-1};
    std::unique_ptr<StrictMock<wrappers::OcdmSessionMock>> m_ocdmSession{
        std::make_unique<StrictMock<wrappers::OcdmSessionMock>>()};
    StrictMock<wrappers::OcdmSessionMock> &m_ocdmSessionMock{*m_ocdmSession};
    firebolt::rialto::wrappers::IOcdmSessionClient *m_client{0};
    std::mutex m_mutex;
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

TEST_F(MediaKeysTest, shouldGenerate)
{
    createMediaKeys();
    sessionWillBeCreated();
    createSession();
    generateRequest();
}

} // namespace firebolt::rialto::server::ct
