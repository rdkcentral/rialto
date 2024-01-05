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
class GetDrmInfoTest : public MediaKeysTestMethods
{
public:
    GetDrmInfoTest() {}
    virtual ~GetDrmInfoTest() {}

    void willGetLdlSessionsLimitRequest();
    void getLdlSessionsLimitRequest();

    void willGetDrmTimeRequest();
    void getDrmTimeRequest();

private:
    const uint32_t m_kTestTime = 34;
    const uint32_t m_kTestLdlLimit = 34;
};

void GetDrmInfoTest::willGetLdlSessionsLimitRequest()
{
    EXPECT_CALL(*m_ocdmSystemMock, getLdlSessionsLimit(_))
        .WillOnce(testing::Invoke(
            [&](uint32_t *ldlLimit) -> MediaKeyErrorStatus
            {
                *ldlLimit = m_kTestLdlLimit;
                return MediaKeyErrorStatus::OK;
            }));
}
void GetDrmInfoTest::getLdlSessionsLimitRequest()
{
    auto request{createGetLdlSessionsLimitRequest(m_mediaKeysHandle)};

    ConfigureAction<GetLdlSessionsLimit>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::GetLdlSessionsLimitResponse &resp)
            {
                EXPECT_EQ(resp.ldl_limit(), m_kTestLdlLimit);
                EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
            });
}

void GetDrmInfoTest::willGetDrmTimeRequest()
{
    EXPECT_CALL(*m_ocdmSystemMock, getDrmTime(_))
        .WillOnce(testing::Invoke(
            [&](uint64_t *time) -> MediaKeyErrorStatus
            {
                *time = m_kTestTime;
                return MediaKeyErrorStatus::OK;
            }));
}
void GetDrmInfoTest::getDrmTimeRequest()
{
    auto request{createGetDrmTimeRequest(m_mediaKeysHandle)};

    ConfigureAction<GetDrmTime>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::GetDrmTimeResponse &resp)
            {
                EXPECT_EQ(resp.drm_time(), m_kTestTime);
                EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
            });
}

/*
 * Component Test: Get various infomation stored in the system.
 * Test Objective:
 *  Test the getLdlSessionsLimit & getDrmTime APIs.
 *
 * Sequence Diagrams:
 *  Get LDL Session Limit, Get DRM Time
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+EME+Misc+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeys
 *
 * Test Initialize:
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Create a MediaKeys object.
 *
 * Test Steps:
 *  Step 1: Get the ldl session limit
 *   Expect that getLdlSessionsLimit is processed by the server.
 *   Api call returns with success.
 *   Check ldl session limit.
 *
 *  Step 2: Get the drm time
 *   Expect that getDrmTime is processed by the server.
 *   Api call returns with success.
 *   Check drm time.
 *
 * Test Teardown:
 *  Destroy MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get both the ldl session limit and DRM time from the MediaKeys object.
 *
 * Code:
 */
TEST_F(GetDrmInfoTest, shouldDrminfo)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Get the ldl session limit
    willGetLdlSessionsLimitRequest();
    getLdlSessionsLimitRequest();

    // Step 2: Get the drm time
    willGetDrmTimeRequest();
    getDrmTimeRequest();
}

} // namespace firebolt::rialto::server::ct
