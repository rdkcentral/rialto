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
class GetSessionInfoTest : public MediaKeysTestMethods
{
public:
    GetSessionInfoTest() {}
    virtual ~GetSessionInfoTest() {}

    void willGetLastDrmError();
    void getLastDrmError();

    void willGetCdmKeySessionId();
    void getCdmKeySessionId();

private:
    const std::string m_kTestString{"Test_str"};
    const uint32_t m_kTestErrorCode{8};
};

void GetSessionInfoTest::willGetLastDrmError()
{
    EXPECT_CALL(m_ocdmSessionMock, getLastDrmError(_))
        .WillOnce(testing::Invoke(
            [&](uint32_t &errorCode) -> MediaKeyErrorStatus
            {
                errorCode = m_kTestErrorCode;
                return MediaKeyErrorStatus::OK;
            }));
}
void GetSessionInfoTest::getLastDrmError()
{
    auto request{createGetLastDrmErrorRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

    ConfigureAction<GetLastDrmError>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::GetLastDrmErrorResponse &resp)
            {
                EXPECT_EQ(resp.error_code(), m_kTestErrorCode);
                EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
            });
}

void GetSessionInfoTest::willGetCdmKeySessionId()
{
    EXPECT_CALL(m_ocdmSessionMock, getCdmKeySessionId(_))
        .WillOnce(testing::Invoke(
            [&](std::string &cdmKeySessionId) -> MediaKeyErrorStatus
            {
                cdmKeySessionId = m_kTestString;
                return MediaKeyErrorStatus::OK;
            }));
}
void GetSessionInfoTest::getCdmKeySessionId()
{
    auto request{createGetCdmKeySessionIdRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

    ConfigureAction<GetCdmKeySessionId>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::GetCdmKeySessionIdResponse &resp)
            {
                EXPECT_EQ(resp.cdm_key_session_id(), m_kTestString);
                EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
            });
}

/*
 * Component Test: Get various information stored in the session.
 * Test Objective:
 *  Test the getLastDrmError & getCdmKeySessionId APIs.
 *
 * Sequence Diagrams:
 *  Get Last DRM Error, Get CDM Key Session Id
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
 * Test Steps:
 *  Step 1: Get the cdm key session id
 *   Expect that getCdmKeySessionId is processed by the server.
 *   Api call returns with success.
 *   Check cdm key session id.
 *
 *  Step 2: Get the last drm error
 *   Expect that getLastDrmError is processed by the server.
 *   Api call returns with success.
 *   Check error code.
 *
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get both the cdm key session id and last drm error from the media key session.
 *
 * Code:
 */
TEST_F(GetSessionInfoTest, getApis)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Get the cdm key session id
    willGetCdmKeySessionId();
    getCdmKeySessionId();

    // Step 2: Get the last drm error
    willGetLastDrmError();
    getLastDrmError();
}

} // namespace firebolt::rialto::server::ct
