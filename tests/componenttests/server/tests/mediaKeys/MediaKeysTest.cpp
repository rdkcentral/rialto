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
#include "Matchers.h"
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

private:
    void createMediaKeys(const ::firebolt::rialto::CreateMediaKeysRequest &request)
    {
        // Use matchResponse to store media keys handle
        ConfigureAction<CreateMediaKeys>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const ::firebolt::rialto::CreateMediaKeysResponse &resp)
                           { m_mediaKeysHandle = resp.media_keys_handle(); });
    }

public:
    void createMediaKeysWidevine() { createMediaKeys(createCreateMediaKeysRequestWidevine()); }

    void createMediaKeysNetflix() { createMediaKeys(createCreateMediaKeysRequestNetflix()); }

    void createKeySession()
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

    void ocdmSessionWillBeCreated()
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

    void generateRequest()
    {
        auto request{createGenerateRequestRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

        bool ok{false};

        // The following should match the details within the message "request"
        EXPECT_CALL(m_ocdmSessionMock,
                    constructSession(KeySessionType::TEMPORARY, InitDataType::CENC, _, request.init_data_size()))
            .WillOnce(testing::Invoke(
                [&](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[],
                    uint32_t initDataSize) -> MediaKeyErrorStatus
                {
                    ok = true;
                    for (uint32_t i = 0; i < initDataSize; ++i)
                    {
                        if (initData[i] != request.init_data(i))
                            ok = false;
                    }
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

        m_ocdmSessionClient->onProcessChallenge(kUrl.c_str(), &kLicenseRequestMessage[0], kLicenseRequestMessage.size());

        myCondVar.wait_for(lock1, std::chrono::milliseconds{110});
        EXPECT_TRUE(callFlag);

        // For teardown...
        EXPECT_CALL(m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::OK));
        EXPECT_CALL(m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));
    }

    void updateSessionWidevine()
    {
        auto request{createUpdateSessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

        bool ok{false};
        // The following should match the details within the message "request"
        EXPECT_CALL(m_ocdmSessionMock, update(_, request.response_data_size()))
            .WillOnce(testing::Invoke(
                [&](const uint8_t response[], uint32_t responseSize) -> MediaKeyErrorStatus
                {
                    ok = true;
                    for (uint32_t i = 0; i < responseSize; ++i)
                    {
                        if (response[i] != request.response_data(i))
                            ok = false;
                    }
                    return MediaKeyErrorStatus::OK;
                }));

        ConfigureAction<UpdateSession>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const firebolt::rialto::UpdateSessionResponse &resp)
                           { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });

        ASSERT_EQ(ok, true);

#if 0
        // TODO(proudmanp): why teardown doesn't work
        // For teardown...
        EXPECT_CALL(m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::OK));
        EXPECT_CALL(m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));
#endif
    }

    void updateSessionNetflix()
    {
        auto request{createUpdateSessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

        bool ok{false};
        // The following should match the details within the message "request"
        EXPECT_CALL(m_ocdmSessionMock, storeLicenseData(_, request.response_data_size()))
            .WillOnce(testing::Invoke(
                [&](const uint8_t challenge[], uint32_t challengeSize) -> MediaKeyErrorStatus
                {
                    ok = true;
                    for (uint32_t i = 0; i < challengeSize; ++i)
                    {
                        if (challenge[i] != request.response_data(i))
                            ok = false;
                    }
                    return MediaKeyErrorStatus::OK;
                }));

        ConfigureAction<UpdateSession>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const firebolt::rialto::UpdateSessionResponse &resp)
                           { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });

        ASSERT_EQ(ok, true);

#if 0
        // TODO(proudmanp): why teardown doesn't work
        // For teardown...
        EXPECT_CALL(m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::OK));
        EXPECT_CALL(m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));
#endif
    }

    void updateAllKeys()
    {
        std::condition_variable myCondVar;
        bool callFlag{false};
        std::unique_lock<std::mutex> lock1(m_mutex);

        std::function<void(const std::shared_ptr<::firebolt::rialto::KeyStatusesChangedEvent> &message)> handler{
            [&](const std::shared_ptr<KeyStatusesChangedEvent> &message)
            {
                std::unique_lock<std::mutex> lock2(m_mutex);

                ASSERT_EQ(message->media_keys_handle(), m_mediaKeysHandle);
                ASSERT_EQ(message->key_session_id(), m_mediaKeySessionId);

#if 0
                // TODO(proudmanp): check the value of this???
                const ::google::protobuf::RepeatedPtrField< ::firebolt::rialto::KeyStatusesChangedEvent_KeyStatusPair >& key_statuses = message-> key_statuses();
#endif

                callFlag = true;
                myCondVar.notify_all();
            }};
        m_clientStub.getIpcChannel()->subscribe(handler);

        m_ocdmSessionClient->onAllKeysUpdated();

        myCondVar.wait_for(lock1, std::chrono::milliseconds{110});
        EXPECT_TRUE(callFlag);
    }

    void updateOneKey()
    {
        const std::vector<unsigned char> kKeyId{'a', 'z', 'q', 'l'};

        // TODO(proudmanp): write a lambda to check the parameters passed in here...
        EXPECT_CALL(m_ocdmSessionMock, getStatus(::arrayMatcher(kKeyId), kKeyId.size())).WillOnce(Return(KeyStatus::USABLE));

        m_ocdmSessionClient->onKeyUpdated(&kKeyId[0], kKeyId.size());
    }

    void licenseRenewal()
    {
        const std::string kUrl{"NOT PASSED TO CALLBACK"};
        const std::vector<unsigned char> kLicenseRenewalMessage{'x', 'u', 'A'};

        std::condition_variable myCondVar;
        bool callFlag{false};
        std::unique_lock<std::mutex> lock1(m_mutex);

        std::function<void(const std::shared_ptr<::firebolt::rialto::LicenseRenewalEvent> &message)> handler{
            [&](const std::shared_ptr<LicenseRenewalEvent> &message)
            {
                std::unique_lock<std::mutex> lock2(m_mutex);

                ASSERT_EQ(message->media_keys_handle(), m_mediaKeysHandle);
                ASSERT_EQ(message->key_session_id(), m_mediaKeySessionId);
                unsigned int max = message->license_renewal_message_size();
                ASSERT_EQ(max, kLicenseRenewalMessage.size());
                for (unsigned int i = 0; i < max; ++i)
                {
                    ASSERT_EQ(message->license_renewal_message(i), kLicenseRenewalMessage[i]);
                }

                callFlag = true;
                myCondVar.notify_all();
            }};
        m_clientStub.getIpcChannel()->subscribe(handler);

        m_ocdmSessionClient->onProcessChallenge(kUrl.c_str(), &kLicenseRenewalMessage[0], kLicenseRenewalMessage.size());

        myCondVar.wait_for(lock1, std::chrono::milliseconds{110});
        EXPECT_TRUE(callFlag);
    }

    void containsKey()
    {
        const std::vector<unsigned char> kKeyId{'a', 'z', 'q', 'l', 'K'};

        auto request{createContainsKeyRequest(m_mediaKeysHandle, m_mediaKeySessionId, kKeyId)};

        EXPECT_CALL(m_ocdmSessionMock, hasKeyId(::arrayMatcher(kKeyId), kKeyId.size())).WillOnce(Return(1));

        ConfigureAction<ContainsKey>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse(
                [&](const firebolt::rialto::ContainsKeyResponse &resp)
                {
                    ASSERT_TRUE(resp.contains_key());
                });
    }

    void doesNotContainKey()
    {
        const std::vector<unsigned char> kKeyId{'a', 'x', 'v'};

        auto request{createContainsKeyRequest(m_mediaKeysHandle, m_mediaKeySessionId, kKeyId)};

        EXPECT_CALL(m_ocdmSessionMock, hasKeyId(::arrayMatcher(kKeyId), kKeyId.size())).WillOnce(Return(0));

        ConfigureAction<ContainsKey>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse(
                [&](const firebolt::rialto::ContainsKeyResponse &resp)
                {
                    ASSERT_FALSE(resp.contains_key());
                });
    }

    void removeKeySession()
    {
        auto request{createRemoveKeySessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

        EXPECT_CALL(m_ocdmSessionMock, remove()).WillOnce(Return(MediaKeyErrorStatus::OK));

        ConfigureAction<RemoveKeySession>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse(
                [&](const firebolt::rialto::RemoveKeySessionResponse &resp)
                {
                    EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
                });
    }

    void loadKeySession()
    {
        auto request{createLoadSessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

        EXPECT_CALL(m_ocdmSessionMock, load()).WillOnce(Return(MediaKeyErrorStatus::OK));

        ConfigureAction<LoadSession>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse(
                [&](const firebolt::rialto::LoadSessionResponse &resp)
                {
                    EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
                });
    }

    void closeKeySessionWidevine()
    {
        auto request{createCloseKeySessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

        EXPECT_CALL(m_ocdmSessionMock, close()).WillOnce(Return(MediaKeyErrorStatus::OK));
        EXPECT_CALL(m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));

        ConfigureAction<CloseKeySession>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse(
                [&](const firebolt::rialto::CloseKeySessionResponse &resp)
                {
                    EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
                });
    }

    void closeKeySessionNetflix()
    {
        auto request{createCloseKeySessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

        EXPECT_CALL(m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::OK));
        EXPECT_CALL(m_ocdmSessionMock, cleanDecryptContext()).WillOnce(Return(MediaKeyErrorStatus::OK));
        EXPECT_CALL(m_ocdmSessionMock, destructSession()).WillOnce(Return(MediaKeyErrorStatus::OK));

        ConfigureAction<CloseKeySession>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse(
                [&](const firebolt::rialto::CloseKeySessionResponse &resp)
                {
                    EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
                });
    }

    void setDrmHeader()
    {
        const std::vector<unsigned char> kKeyId{'a', 'j', 'l'};

        auto request{createSetDrmHeaderRequest(m_mediaKeysHandle, m_mediaKeySessionId, kKeyId)};

        EXPECT_CALL(m_ocdmSessionMock, setDrmHeader(::arrayMatcher(kKeyId), kKeyId.size())).WillOnce(Return(MediaKeyErrorStatus::OK));

        ConfigureAction<SetDrmHeader>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse(
                [&](const firebolt::rialto::SetDrmHeaderResponse &resp)
                {
                    EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
                });
    }

    void getLastDrmError()
    {
        auto request{createGetLastDrmErrorRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

        const uint32_t kTestErrorCode{8};

        EXPECT_CALL(m_ocdmSessionMock, getLastDrmError(_)).WillOnce(testing::Invoke(
                [&](uint32_t &errorCode) -> MediaKeyErrorStatus
                {
                    errorCode = kTestErrorCode;
                    return MediaKeyErrorStatus::OK;
                }));

        ConfigureAction<GetLastDrmError>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse(
                [&](const firebolt::rialto::GetLastDrmErrorResponse &resp)
                {
                    EXPECT_EQ(resp.error_code(), kTestErrorCode);
                    EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
                });
    }

    int m_mediaKeysHandle{-1};
    int m_mediaKeySessionId{-1};
    std::unique_ptr<StrictMock<wrappers::OcdmSessionMock>> m_ocdmSession{
        std::make_unique<StrictMock<wrappers::OcdmSessionMock>>()};
    StrictMock<wrappers::OcdmSessionMock> &m_ocdmSessionMock{*m_ocdmSession};
    firebolt::rialto::wrappers::IOcdmSessionClient *m_ocdmSessionClient{0};
    std::mutex m_mutex;
};

TEST_F(MediaKeysTest, shouldFailToCreateSessionWhenMksIdIsWrong)
{
    createMediaKeysWidevine();
    auto request{createCreateKeySessionRequest(m_mediaKeysHandle + 1)};
    ConfigureAction<CreateKeySession>(m_clientStub)
        .send(request)
        .expectSuccess() // sick!
        .matchResponse([&](const firebolt::rialto::CreateKeySessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::FAIL); });
}

/*
 * Component Test:
 * Test Objective:
 *
 *
 * Sequence Diagrams:
 *
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: RialtoApplicationSessionServer with stubs for RialtoClient and RialtoServerManager
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts the application server running in its own thread
 *
 *
 * Test Steps:
 *  Step A1:
 *
 *
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *
 * Code:
 */
TEST_F(MediaKeysTest, shouldGenerate)
{
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();
    createKeySession();
    generateRequest();
}

TEST_F(MediaKeysTest, shouldUpdateWidevineAllKeys)
{
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();
    createKeySession();
    updateSessionWidevine();
    updateAllKeys();
    licenseRenewal();
}

TEST_F(MediaKeysTest, shouldUpdateWidevineOneKey)
{
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();
    createKeySession();
    updateSessionWidevine();
    updateOneKey();
    licenseRenewal();
    closeKeySessionWidevine();
}

TEST_F(MediaKeysTest, shouldUpdatNetflixAllKeys)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();
    updateSessionNetflix();
    updateAllKeys();
    licenseRenewal();
}

TEST_F(MediaKeysTest, shouldUpdatNetflixOneKey)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();
    updateSessionNetflix();
    updateOneKey();
    licenseRenewal();
    containsKey();
    doesNotContainKey();
    loadKeySession();
    removeKeySession();
    setDrmHeader();
    getLastDrmError();
    closeKeySessionNetflix();
}

} // namespace firebolt::rialto::server::ct
