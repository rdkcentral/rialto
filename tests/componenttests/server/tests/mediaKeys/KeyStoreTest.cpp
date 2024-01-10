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

#include <vector>

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Matchers.h"
#include "MediaKeysTestMethods.h"

using testing::_;
using testing::ByMove;
using testing::Return;
using testing::StrictMock;

namespace firebolt::rialto::server::ct
{
class KeyStoreTest : public MediaKeysTestMethods
{
public:
    KeyStoreTest() {}
    virtual ~KeyStoreTest() {}

    void willGetKeyStoreHashRequest();
    void getKeyStoreHashRequest();

    void willGetKeyStoreHashRequestFails();
    void getKeyStoreHashRequestFails();

    void willDeleteKeyStoreRequest();
    void deleteKeyStoreRequest();

private:
    const std::vector<unsigned char> m_kHashTest1{'d', 'z', 'f'};
};
void KeyStoreTest::willGetKeyStoreHashRequest()
{
    EXPECT_CALL(*m_ocdmSystemMock, getKeyStoreHash(_, _))
        .WillOnce(testing::Invoke(
            [&](uint8_t keyStoreHash[], uint32_t keyStoreHashLength) -> MediaKeyErrorStatus
            {
                // The real wrapper calls opencdm_get_key_store_hash_ext()
                // defined in opencdm/opencdm_ext.h
                // and this header specifies the length should be at least 64
                // (but doesn't return the number of bytes actually filled)
                EXPECT_GT(keyStoreHashLength, 64);
                size_t i = 0;
                for (; i < m_kHashTest1.size(); ++i)
                {
                    keyStoreHash[i] = m_kHashTest1[i];
                }
                // Pad with zeros in case valgrind complains...
                for (; i < keyStoreHashLength; ++i)
                {
                    keyStoreHash[i] = 0;
                }
                return MediaKeyErrorStatus::OK;
            }));
}
void KeyStoreTest::getKeyStoreHashRequest()
{
    auto request{createGetKeyStoreHashRequest(m_mediaKeysHandle)};

    ConfigureAction<GetKeyStoreHash>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::GetKeyStoreHashResponse &resp)
            {
                EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
                for (size_t i = 0; i < m_kHashTest1.size(); ++i)
                {
                    EXPECT_EQ(resp.key_store_hash(i), m_kHashTest1[i]);
                }
            });
}

void KeyStoreTest::willGetKeyStoreHashRequestFails()
{
    EXPECT_CALL(*m_ocdmSystemMock, getKeyStoreHash(_, _)).WillOnce(Return(MediaKeyErrorStatus::FAIL));
}
void KeyStoreTest::getKeyStoreHashRequestFails()
{
    auto request{createGetKeyStoreHashRequest(m_mediaKeysHandle)};

    ConfigureAction<GetKeyStoreHash>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GetKeyStoreHashResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::FAIL); });
}

void KeyStoreTest::willDeleteKeyStoreRequest()
{
    EXPECT_CALL(*m_ocdmSystemMock, deleteKeyStore()).WillOnce(Return(MediaKeyErrorStatus::OK));
}
void KeyStoreTest::deleteKeyStoreRequest()
{
    auto request{createDeleteKeyStoreRequest(m_mediaKeysHandle)};

    ConfigureAction<DeleteKeyStore>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::DeleteKeyStoreResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

/*
 * Component Test: Key Store APIs.
 * Test Objective:
 *  Test the deleteKeyStore and getKeyStoreHash APIs.
 *
 * Sequence Diagrams:
 *  Delete Key Store, Get Key Store Hash
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+EME+Misc+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeys
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts rialtoServer running in its own thread
 *   send a CreateMediaKeys message to rialtoServer
 *   expect a "createSession" call (to OCDM mock)
 *   send a CreateKeySession message to rialtoServer
 *
 *
 * Test Steps:
 *  Step 1: Get the key store
 *   getKeyStoreHash.
 *   Expect that getKeyStoreHash is processed by the server.
 *   Api call returns with success.
 *   Check key store hash.
 *
 *  Step 2: Delete the key store
 *   deleteKeyStore.
 *   Expect that deleteKeyStore is processed by the server.
 *   Api call returns with success.
 *
 *  Step 3: Get the key store failure
 *   getKeyStoreHash.
 *   Expect that getKeyStoreHash is processed by the server.
 *   Api call returns with failure.
 *
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get and delete the key store successfully.
 *
 * Code:
 */
TEST_F(KeyStoreTest, shouldKeystore)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Get the key store
    willGetKeyStoreHashRequest();
    getKeyStoreHashRequest();

    // Step 2: Delete the key store
    willDeleteKeyStoreRequest();
    deleteKeyStoreRequest();

    // Step 3: Get the key store failure
    willGetKeyStoreHashRequestFails();
    getKeyStoreHashRequestFails();
}
} // namespace firebolt::rialto::server::ct
