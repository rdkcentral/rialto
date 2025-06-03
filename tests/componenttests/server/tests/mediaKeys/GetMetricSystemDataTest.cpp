/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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
class GetMetricSystemDataTest : public MediaKeysTestMethods
{
public:
    GetMetricSystemDataTest() {}
    virtual ~GetMetricSystemDataTest() {}

    void willGetMetricSystemData();
    void getMetricSystemData();

    void willFailGetMetricSystemData();
    void getMetricSystemDataFails();

    void willResizeBufferAndSucceed();
    void getMetricSystemDataAfterResize();

    const std::vector<uint8_t> m_kBuffer{0x1, 0x2, 0x3, 0x4};
};

void GetMetricSystemDataTest::willGetMetricSystemData()
{
    EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_, _))
        .WillOnce(testing::Invoke(
            [&](uint32_t &bufferLength, std::vector<uint8_t> &buffer) -> MediaKeyErrorStatus
            {
                bufferLength = m_kBuffer.size();
                buffer = m_kBuffer;
                return MediaKeyErrorStatus::OK;
            }));
}

void GetMetricSystemDataTest::getMetricSystemData()
{
    auto request{createGetMetricSystemDataRequest(m_mediaKeysHandle)};

    ConfigureAction<GetMetricSystemData>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GetMetricSystemDataResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void GetMetricSystemDataTest::willFailGetMetricSystemData()
{
    EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_, _))
        .WillOnce(testing::Invoke(
            [&](uint32_t &bufferLength, std::vector<uint8_t> &buffer) -> MediaKeyErrorStatus
            {
                bufferLength = m_kBuffer.size();
                buffer = m_kBuffer;
                return MediaKeyErrorStatus::FAIL;
            }));
}
void GetMetricSystemDataTest::getMetricSystemDataFails()
{
    auto request{createGetMetricSystemDataRequest(m_mediaKeysHandle)};

    ConfigureAction<GetMetricSystemData>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GetMetricSystemDataResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::FAIL); });
}

/**
 * Component Test: Get Metric System Data API.
 * Test Objective:
 *  Test getMetricSystemDataTest API.
 *
 * Sequence Diagrams:
 *  Get Metric System Data
 *   -
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
 *
 * Step 1: Get metric system data success
 *   Expect that getMetricSystemData is processed by the server.
 *   Api call returns with success.
 *   Check status.
 *
 * Step 2: Get metric system data failure
 *   Expect that getMetricSystemData is processed by the server.
 *   Api call returns with failure.
 *
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get metric system data successfully.
 *
 * Code:
 */
TEST_F(GetMetricSystemDataTest, shouldGetMetricSystemData)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Get metric system data success
    willGetMetricSystemData();
    getMetricSystemData();

    // Step 2: Get metric system data failure
    willFailGetMetricSystemData();
    getMetricSystemDataFails();
}
}; // namespace firebolt::rialto::server::ct
