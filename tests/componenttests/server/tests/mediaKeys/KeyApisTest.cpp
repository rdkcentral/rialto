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
class KeyApisTest : public MediaKeysTestMethods
{
public:
    KeyApisTest() {}
    virtual ~KeyApisTest() {}

    void willContainsKey();
    void containsKey();

    void willDoesNotContainKey();
    void doesNotContainKey();

    void willRemoveKeySession();
    void removeKeySession();

    void willLoadKeySession();
    void loadKeySession();

    const std::vector<unsigned char> m_kKeyId1{'a', 'z', 'q', 'l', 'K'};
    const std::vector<unsigned char> m_kKeyId2{'a', 'x', 'v'};
};

void KeyApisTest::willContainsKey()
{
    EXPECT_CALL(m_ocdmSessionMock, hasKeyId(::arrayMatcher(m_kKeyId1), m_kKeyId1.size())).WillOnce(Return(1));
}

void KeyApisTest::containsKey()
{
    auto request{createContainsKeyRequest(m_mediaKeysHandle, m_mediaKeySessionId, m_kKeyId1)};

    ConfigureAction<ContainsKey>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::ContainsKeyResponse &resp) { ASSERT_TRUE(resp.contains_key()); });
}

void KeyApisTest::willDoesNotContainKey()
{
    EXPECT_CALL(m_ocdmSessionMock, hasKeyId(::arrayMatcher(m_kKeyId2), m_kKeyId2.size())).WillOnce(Return(0));
}
void KeyApisTest::doesNotContainKey()
{
    auto request{createContainsKeyRequest(m_mediaKeysHandle, m_mediaKeySessionId, m_kKeyId2)};

    ConfigureAction<ContainsKey>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::ContainsKeyResponse &resp) { ASSERT_FALSE(resp.contains_key()); });
}

void KeyApisTest::willRemoveKeySession()
{
    EXPECT_CALL(m_ocdmSessionMock, remove()).WillOnce(Return(MediaKeyErrorStatus::OK));
}
void KeyApisTest::removeKeySession()
{
    auto request{createRemoveKeySessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

    ConfigureAction<RemoveKeySession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::RemoveKeySessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void KeyApisTest::willLoadKeySession()
{
    EXPECT_CALL(m_ocdmSessionMock, load()).WillOnce(Return(MediaKeyErrorStatus::OK));
}
void KeyApisTest::loadKeySession()
{
    auto request{createLoadSessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

    ConfigureAction<LoadSession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::LoadSessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

/*
 * Component Test: Key APIs.
 * Test Objective:
 *  Test the key session management apis containsKey, loadKeySession & removeKeySession,
 *  that are not tested in the normal media key session sequence.
 *
 * Sequence Diagrams:
 *  Contains Key, Load/OCDM, Remove/OCDM
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Media+Key+Session+Management+Design
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
 *  Step 1: Load session
 *   loadSession of the closed session.
 *   Expect that loadSession is processed by the server.
 *   Api call returns with success.
 *
 *  Step 2: Does not contain key
 *   containsKey with key id that has not been set.
 *   Expect that containsKey is processed by the server.
 *   containsKey returns false.
 *
 *  Step 3: Does contain key
 *   containsKey with key id that has been selected.
 *   Expect that containsKey is processed by the server.
 *   containsKey returns true.
 *
 *  Step 4: Remove session
 *   removeKeySession.
 *   Expect that removeKeySession is processed by the server.
 *   Api call returns with success.
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *   Load a closed key session successfully.
 *   Query if keys exist or not in the session using containsKey.
 *   Remove key session successfully.
 *
 * Code:
 */
TEST_F(KeyApisTest, keyManagement)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Load session
    willLoadKeySession();
    loadKeySession();

    // Step 2: Does not contain key
    willDoesNotContainKey();
    doesNotContainKey();

    // Step 3: Does contain key
    willContainsKey();
    containsKey();

    // Step 4: Remove session
    willRemoveKeySession();
    removeKeySession();
}

} // namespace firebolt::rialto::server::ct
