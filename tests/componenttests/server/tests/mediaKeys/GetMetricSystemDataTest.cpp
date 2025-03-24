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

    // void willResizeBufferAndSucceed();
    // void getMetricSystemDataAfterResize();

    // void willGetMetricSystemDataInterfaceNotImplemented();
    // void getMetricSystemDataInterfaceNotImplemented();

    const std::vector<uint8_t> m_kBuffer{0x91, 0x2E, 0x5D, 0xF3};
};

void GetMetricSystemDataTest::willGetMetricSystemData()
{
    EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_,_))
        .WillOnce(testing::Invoke([&](uint32_t* bufferLength, std::vector<uint8_t>* buffer) -> MediaKeyErrorStatus {
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
        .matchResponse(
            [&](const firebolt::rialto::GetMetricSystemDataResponse &resp)
            {
                EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
                for (size_t i = 0; i < m_kBuffer.size(); ++i)
                {
                    EXPECT_EQ(resp.buffer(i), m_kBuffer[i]);
                }
            });
}

void GetMetricSystemDataTest::willFailGetMetricSystemData()
{
    EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_,_))
        .WillOnce(testing::Invoke([&](uint32_t* bufferLength, std::vector<uint8_t>* buffer) -> MediaKeyErrorStatus {
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

// void GetMetricSystemDataTest::willResizeBufferAndSucceed()
// {
//     EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_,_))
//         .WillOnce(testing::Invoke([&](uint32_t* bufferLength, std::vector<uint8_t>* buffer) -> MediaKeyErrorStatus {
//             *bufferLength = m_kBuffer.size();
//             *buffer = m_kBuffer;
//             return MediaKeyErrorStatus::BUFFER_TOO_SMALL;
//         }))
//         .WillOnce(testing::Invoke([&](uint32_t* bufferLength, std::vector<uint8_t>* buffer) -> MediaKeyErrorStatus {
//             *bufferLength = m_kBuffer.size();
//             *buffer = m_kBuffer;
//             return MediaKeyErrorStatus::OK;
//         }));
// }

// void GetMetricSystemDataTest::getMetricSystemDataAfterResize()
// {
//     auto request{createGetMetricSystemDataRequest(m_mediaKeysHandle)};

//     ConfigureAction<GetMetricSystemData>(m_clientStub)
//         .send(request)
//         .expectSuccess()
//         .matchResponse(
//             [&](const firebolt::rialto::GetMetricSystemDataResponse &resp)
//             {  
//                 EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
//                 ASSERT_EQ(resp.buffer_size(), m_kBuffer.size());
//                 for (size_t i = 0; i < m_kBuffer.size(); ++i)
//                 {
//                     EXPECT_EQ(resp.buffer(i), m_kBuffer[i]);
//                 }
//             });
// }

// void GetMetricSystemDataTest::willGetMetricSystemDataInterfaceNotImplemented()
// {
//     EXPECT_CALL(*m_ocdmSystemMock, getMetricSystemData(_,_))
//         .WillOnce(testing::Invoke([&](uint32_t* bufferLength, std::vector<uint8_t>* buffer) -> MediaKeyErrorStatus {
//             *bufferLength = m_kBuffer.size();
//             *buffer = m_kBuffer;
//             return MediaKeyErrorStatus::INTERFACE_NOT_IMPLEMENTED;
//         }));
// }

// void GetMetricSystemDataTest::getMetricSystemDataInterfaceNotImplemented()
// {
//     auto request{createGetMetricSystemDataRequest(m_mediaKeysHandle)};

//     ConfigureAction<GetMetricSystemData>(m_clientStub)
//         .send(request)
//         .expectSuccess()
//         .matchResponse([&](const firebolt::rialto::GetMetricSystemDataResponse &resp)
//                        { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::INTERFACE_NOT_IMPLEMENTED); });

// }

/** 
 * Component Test: Drm Store APIs.
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

 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get and delete the drm store successfully.
 *
 * Code:
 */
TEST_F(GetMetricSystemDataTest, shouldGetMetricSystemData)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    willGetMetricSystemData();
    getMetricSystemData();

    willFailGetMetricSystemData();
    getMetricSystemDataFails();

    // willResizeBufferAndSucceed();
    // getMetricSystemDataAfterResize();

    // willGetMetricSystemDataInterfaceNotImplemented();
    // getMetricSystemDataInterfaceNotImplemented();
} 
};// namespace firebolt::rialto::server::ct
