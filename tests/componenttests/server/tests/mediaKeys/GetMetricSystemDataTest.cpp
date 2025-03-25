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

    const std::vector<uint8_t>
        m_kBuffer{0x0A, 0xE1, 0x01, 0x0A, 0xAA, 0x01, 0x4A, 0x06, 0x08, 0x01, 0x12, 0x02, 0x08, 0x00, 0x52, 0x09, 0x19,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0xE6, 0xAF, 0x40, 0x6A, 0x04, 0x10, 0x9E, 0x92, 0x01, 0x82, 0x01, 0x02,
                  0x10, 0x10, 0x8A, 0x01, 0x06, 0x08, 0x01, 0x12, 0x02, 0x48, 0x00, 0xDA, 0x01, 0x06, 0x08, 0x01, 0x12,
                  0x02, 0x48, 0x00, 0xA2, 0x02, 0x06, 0x08, 0x01, 0x12, 0x02, 0x48, 0x00, 0xAA, 0x02, 0x06, 0x08, 0x03,
                  0x12, 0x02, 0x48, 0x00, 0xB2, 0x02, 0x0B, 0x1D, 0x00, 0xE0, 0xF8, 0x44, 0x28, 0x01, 0x32, 0x02, 0x48,
                  0x00, 0xFA, 0x02, 0x02, 0x10, 0x03, 0xA2, 0x03, 0x02, 0x10, 0x00, 0xB2, 0x03, 0x02, 0x10, 0x01, 0xD2,
                  0x03, 0x02, 0x10, 0x00, 0xF2, 0x03, 0x06, 0x08, 0x01, 0x12, 0x02, 0x48, 0x00, 0xF2, 0x03, 0x06, 0x08,
                  0x01, 0x12, 0x02, 0x48, 0x07, 0xFA, 0x03, 0x06, 0x08, 0x01, 0x12, 0x02, 0x48, 0x29, 0xC2, 0x04, 0x02,
                  0x10, 0x01, 0xFA, 0x04, 0x0B, 0x1D, 0x00, 0x7D, 0x05, 0x47, 0x28, 0x01, 0x32, 0x02, 0x48, 0x00, 0xFA,
                  0x04, 0x0B, 0x1D, 0x00, 0x80, 0x9A, 0x43, 0x28, 0x01, 0x32, 0x02, 0x48, 0x07, 0x9A, 0x05, 0x02, 0x10,
                  0x03, 0xA2, 0x05, 0x02, 0x10, 0x00, 0x42, 0x08, 0x22, 0x06, 0x31, 0x36, 0x2E, 0x34, 0x2E, 0x30, 0x52,
                  0x06, 0x10, 0x86, 0xBF, 0x85, 0xBF, 0x06, 0x72, 0x0B, 0x1D, 0x00, 0x65, 0x37, 0x47, 0x28, 0x01, 0x32,
                  0x02, 0x08, 0x00, 0x8A, 0x01, 0x0B, 0x1D, 0x00, 0x00, 0x18, 0x44, 0x28, 0x01, 0x32, 0x02, 0x08, 0x2F,
                  0x92, 0x01, 0x04, 0x10, 0xB7, 0x85, 0x25, 0x12, 0xBF, 0x01, 0x12, 0x86, 0x01, 0x0A, 0x02, 0x10, 0x01,
                  0x52, 0x09, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x62, 0x0D, 0x1D, 0x00, 0x80, 0x1D,
                  0x46, 0x28, 0x01, 0x32, 0x04, 0x08, 0x00, 0x20, 0x00, 0x6A, 0x04, 0x10, 0x9E, 0x92, 0x01, 0x82, 0x01,
                  0x02, 0x10, 0x10, 0x8A, 0x01, 0x06, 0x08, 0x01, 0x12, 0x02, 0x48, 0x00, 0xA2, 0x02, 0x06, 0x08, 0x01,
                  0x12, 0x02, 0x48, 0x00, 0xAA, 0x02, 0x06, 0x08, 0x01, 0x12, 0x02, 0x48, 0x00, 0xB2, 0x02, 0x0B, 0x1D,
                  0x80, 0x6A, 0xB9, 0x47, 0x28, 0x01, 0x32, 0x02, 0x48, 0x00, 0xFA, 0x02, 0x02, 0x10, 0x03, 0xB2, 0x03,
                  0x02, 0x10, 0x01, 0xD2, 0x03, 0x02, 0x10, 0x00, 0xF2, 0x03, 0x06, 0x08, 0x01, 0x12, 0x02, 0x48, 0x00,
                  0xF2, 0x03, 0x06, 0x08, 0x01, 0x12, 0x02, 0x48, 0x07, 0xFA, 0x03, 0x06, 0x08, 0x01, 0x12, 0x02, 0x48,
                  0x29, 0x9A, 0x05, 0x02, 0x10, 0x03, 0xA2, 0x05, 0x02, 0x10, 0x00, 0x1A, 0x09, 0x19, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x24, 0x40, 0x42, 0x29, 0x22, 0x27, 0x4F, 0x45, 0x4D, 0x43, 0x72, 0x79, 0x70, 0x74,
                  0x6F, 0x20, 0x52, 0x65, 0x66, 0x20, 0x43, 0x6F, 0x64, 0x65, 0x20, 0x4A, 0x61, 0x6E, 0x20, 0x20, 0x37,
                  0x20, 0x32, 0x30, 0x32, 0x35, 0x20, 0x31, 0x38, 0x3A, 0x32, 0x34, 0x3A, 0x34, 0x39};
};

void GetMetricSystemDataTest::willGetMetricSystemData()
{
    EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_, _))
        .WillOnce(testing::Invoke(
            [&](uint32_t *bufferLength, std::vector<uint8_t> *buffer) -> MediaKeyErrorStatus
            {
                *bufferLength = m_kBuffer.size();
                *buffer = m_kBuffer;
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
            [&](uint32_t *bufferLength, std::vector<uint8_t> *buffer) -> MediaKeyErrorStatus
            {
                *bufferLength = m_kBuffer.size();
                *buffer = m_kBuffer;
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
