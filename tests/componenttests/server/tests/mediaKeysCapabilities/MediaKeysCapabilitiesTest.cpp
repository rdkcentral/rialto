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
#include "MediaKeysCommon.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"

using testing::_;
using testing::ByMove;
using testing::Return;
using testing::StrictMock;
namespace firebolt::rialto::server::ct
{
class MediaKeysCapabilitiesTest : public RialtoServerComponentTest
{
public:
    MediaKeysCapabilitiesTest()
    {
        willConfigureSocket();
        configureSutInActiveState();
        connectClient();
    }
    virtual ~MediaKeysCapabilitiesTest() {}

    void willGetSupportedKeySystemsWithAllSupported();
    void getSupportedKeySystemsWithAllSupported();

    void willGetSupportedKeySystemsWithOneUnsupported();
    void getSupportedKeySystemsWithOneUnsupported();

    void willGetSupportedKeySystemsWithNoneSupported();
    void getSupportedKeySystemsWithNoneSupported();

    void willCallSupportsKeySystem();
    void callSupportsKeySystem();

    void willCallSupportKeySystemWithResponseNo();
    void callSupportKeySystemWithResponseNo();

    void willGetSupportedKeySystemVersionRequest1();
    void getSupportedKeySystemVersionRequest1();

    void willGetSupportedKeySystemVersionRequest2();
    void getSupportedKeySystemVersionRequest2();

    void willGetSupportedKeySystemVersionRequestFail();
    void getSupportedKeySystemVersionRequestFail();

private:
    std::shared_ptr<testing::StrictMock<wrappers::OcdmSystemMock>> m_alternateOcdmSystemMock{
        std::make_shared<testing::StrictMock<wrappers::OcdmSystemMock>>()};
    int m_nextKeyTypeIndex{0};

    const std::string m_kKeySystemFail = "com.unknown";
    const std::string m_kTestVersion1{"7.5.test"};
    const std::string m_kTestVersion2{"3.11.testAlternate"};
};

void MediaKeysCapabilitiesTest::willGetSupportedKeySystemsWithAllSupported()
{
    m_nextKeyTypeIndex = 0;
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(_))
        .Times(kSupportedKeySystems.size())
        .WillRepeatedly(testing::Invoke(
            [&](const std::string &keySystem) -> MediaKeyErrorStatus
            {
                // Check server will repeatedly call OCDM isTypeSupported
                // for every key system that rialto supports
                EXPECT_LT(m_nextKeyTypeIndex, kSupportedKeySystems.size());
                EXPECT_EQ(keySystem, kSupportedKeySystems[m_nextKeyTypeIndex]);
                m_nextKeyTypeIndex++;
                return MediaKeyErrorStatus::OK;
            }));
}

void MediaKeysCapabilitiesTest::getSupportedKeySystemsWithAllSupported()
{
    auto request = createGetSupportedKeySystemsRequest();
    ConfigureAction<GetSupportedKeySystems>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::GetSupportedKeySystemsResponse &resp)
            {
                // Check that the response contains all key systems
                int max = resp.key_systems_size();
                EXPECT_EQ(max, kSupportedKeySystems.size());
                for (int i = 0; i < max; ++i)
                {
                    EXPECT_EQ(kSupportedKeySystems[i], resp.key_systems(i));
                }
            });
}

void MediaKeysCapabilitiesTest::willGetSupportedKeySystemsWithOneUnsupported()
{
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(_))
        .Times(kSupportedKeySystems.size())
        .WillRepeatedly(testing::Invoke(
            [&](const std::string &keySystem) -> MediaKeyErrorStatus
            {
                if (keySystem == kPlayreadyKeySystem)
                    return MediaKeyErrorStatus::FAIL;
                return MediaKeyErrorStatus::OK;
            }));
}

void MediaKeysCapabilitiesTest::getSupportedKeySystemsWithOneUnsupported()
{
    auto request = createGetSupportedKeySystemsRequest();
    ConfigureAction<GetSupportedKeySystems>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::GetSupportedKeySystemsResponse &resp)
            {
                // Check that the response contains all key systems except kPlayreadyKeySystem
                size_t j = 0;
                size_t maxJ = kSupportedKeySystems.size();

                int i = 0;
                int maxI = resp.key_systems_size();

                for (; i < maxI && j < maxJ; ++i, ++j)
                {
                    if (kSupportedKeySystems[j] == kPlayreadyKeySystem)
                        ++j;
                    EXPECT_EQ(kSupportedKeySystems[j], resp.key_systems(i));
                }
                EXPECT_EQ(i, maxI);
                EXPECT_EQ(j, maxJ);
            });
}

void MediaKeysCapabilitiesTest::willGetSupportedKeySystemsWithNoneSupported()
{
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(_))
        .Times(kSupportedKeySystems.size())
        .WillRepeatedly(Return(MediaKeyErrorStatus::FAIL));
}

void MediaKeysCapabilitiesTest::getSupportedKeySystemsWithNoneSupported()
{
    auto request = createGetSupportedKeySystemsRequest();
    ConfigureAction<GetSupportedKeySystems>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::GetSupportedKeySystemsResponse &resp)
            {
                // Check that the response contains NO key systems
                EXPECT_EQ(resp.key_systems_size(), 0);
            });
}

void MediaKeysCapabilitiesTest::willCallSupportsKeySystem()
{
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(kPlayreadyKeySystem)).WillOnce(Return(MediaKeyErrorStatus::OK));
}

void MediaKeysCapabilitiesTest::callSupportsKeySystem()
{
    auto request = createSupportsKeySystemRequest(kPlayreadyKeySystem);

    ConfigureAction<SupportsKeySystem>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::SupportsKeySystemResponse &resp)
                       { EXPECT_TRUE(resp.is_supported()); });
}

void MediaKeysCapabilitiesTest::willCallSupportKeySystemWithResponseNo()
{
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(m_kKeySystemFail)).WillOnce(Return(MediaKeyErrorStatus::FAIL));
}

void MediaKeysCapabilitiesTest::callSupportKeySystemWithResponseNo()
{
    auto request = createSupportsKeySystemRequest(m_kKeySystemFail);

    ConfigureAction<SupportsKeySystem>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::SupportsKeySystemResponse &resp)
                       { EXPECT_FALSE(resp.is_supported()); });
}

void MediaKeysCapabilitiesTest::willGetSupportedKeySystemVersionRequest1()
{
    // see the code in MediaKeysCapabilities::getSupportedKeySystemVersion()
    // This is how the ocdm library changes context to this keysystem before getting the version
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kNetflixKeySystem)).WillOnce(Return(m_ocdmSystemMock));

    EXPECT_CALL(*m_ocdmSystemMock, getVersion(_))
        .WillOnce(testing::Invoke(
            [&](std::string &version) -> MediaKeyErrorStatus
            {
                version = m_kTestVersion1;
                return MediaKeyErrorStatus::OK;
            }));
}

void MediaKeysCapabilitiesTest::getSupportedKeySystemVersionRequest1()
{
    auto request = createGetSupportedKeySystemVersionRequest(kNetflixKeySystem);

    ConfigureAction<GetSupportedKeySystemVersion>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GetSupportedKeySystemVersionResponse &resp)
                       { EXPECT_EQ(resp.version(), m_kTestVersion1); });
}

void MediaKeysCapabilitiesTest::willGetSupportedKeySystemVersionRequest2()
{
    // There should be NO other pointer to m_alternateOcdmSystemMock
    EXPECT_EQ(m_alternateOcdmSystemMock.use_count(), 1);

    // see the code in MediaKeysCapabilities::getSupportedKeySystemVersion()
    // This is how the ocdm library changes context to this keysystem before getting the version
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kPlayreadyKeySystem)).WillOnce(Return(m_alternateOcdmSystemMock));

    EXPECT_CALL(*m_alternateOcdmSystemMock, getVersion(_))
        .WillOnce(testing::Invoke(
            [&](std::string &version) -> MediaKeyErrorStatus
            {
                version = m_kTestVersion2;

                // The rialto system should be holding a shared pointer
                // to m_alternateOcdmSystemMock at this time
                EXPECT_GT(m_alternateOcdmSystemMock.use_count(), 1);
                return MediaKeyErrorStatus::OK;
            }));
}

void MediaKeysCapabilitiesTest::getSupportedKeySystemVersionRequest2()
{
    auto request2 = createGetSupportedKeySystemVersionRequest(kPlayreadyKeySystem);
    ConfigureAction<GetSupportedKeySystemVersion>(m_clientStub)
        .send(request2)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GetSupportedKeySystemVersionResponse &resp)
                       { EXPECT_EQ(resp.version(), m_kTestVersion2); });

    // This tests that the Rialto System releases all shared pointers to the
    // Ocdm System, and therefore it would be destructed (if the test environment
    // didn't keep a pointer)
    EXPECT_EQ(m_alternateOcdmSystemMock.use_count(), 1);
}

void MediaKeysCapabilitiesTest::willGetSupportedKeySystemVersionRequestFail()
{
    // see the code in MediaKeysCapabilities::getSupportedKeySystemVersion()
    // This is how the ocdm library changes context to this keysystem before getting the version
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kNetflixKeySystem)).WillOnce(Return(m_ocdmSystemMock));

    EXPECT_CALL(*m_ocdmSystemMock, getVersion(_)).WillOnce(Return(MediaKeyErrorStatus::FAIL));
}

void MediaKeysCapabilitiesTest::getSupportedKeySystemVersionRequestFail()
{
    auto request = createGetSupportedKeySystemVersionRequest(kNetflixKeySystem);

    ConfigureAction<GetSupportedKeySystemVersion>(m_clientStub).send(request).expectFailure();
}

/*
 * Component Test: MediaKeyCapabilities API
 * Test Objective:
 *  Test the server component's response to getSupportedKeySystems
 *  where OCDM system doesn't support kPlayreadyKeySystem
 *
 * Sequence Diagrams:
 *   1: Check Supported Key Systems - indicative use of Rialto
 *   From: https://wiki.rdkcentral.com/pages/viewpage.action?pageId=226375556
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeysCapabilities with stubs for RialtoClient and ocdm
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts rialtoServer running in its own thread
 *
 * Test Steps:
 *  Step A1: Get the supported key systems
 *   Expect that getSupportedKeySystems is propagated to the server from client
 *   The server will repeatedly call OCDM isTypeSupported() for every
 *   key system that rialto supports, but OCDM will return false for the
 *   kPlayreadyKeySystem
 *   Check the array of key systems returned to the client contains everything
 *   except kPlayreadyKeySystem
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *    The tests detailed in Step 1 should be successful
 *
 * Code:
 */
TEST_F(MediaKeysCapabilitiesTest, philTest2)
{
    // Step A1: Get the supported key systems
    willGetSupportedKeySystemsWithOneUnsupported();
    getSupportedKeySystemsWithOneUnsupported();
}

/*
 * Component Test: MediaKeyCapabilities API
 * Test Objective:
 *  Test the server component's response to getSupportedKeySystems
 *  where OCDM system doesn't support kPlayreadyKeySystem
 *
 * Sequence Diagrams:
 *   1: Check Supported Key Systems - indicative use of Rialto
 *   From: https://wiki.rdkcentral.com/pages/viewpage.action?pageId=226375556
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeysCapabilities with stubs for RialtoClient and ocdm
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts rialtoServer running in its own thread
 *
 * Test Steps:
 *  Step B1: Get no supported key systems
 *   Expect that getSupportedKeySystems is propagated to the server from client
 *   The server will repeatedly call OCDM isTypeSupported() for every
 *   key system that rialto supports, but OCDM will return false for all
 *   Check the array of key systems returned to the client contains nothing
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *    The tests detailed in Step 1 should be successful
 *
 * Code:
 */
TEST_F(MediaKeysCapabilitiesTest, philTest3)
{
    // Step B1: Get no supported key systems
    willGetSupportedKeySystemsWithNoneSupported();
    getSupportedKeySystemsWithNoneSupported();
}

/*
 * Component Test: MediaKeyCapabilities API
 * Test Objective:
 *  Test the server component's response to getSupportedKeySystems,
 *  supportsKeySystem and getSupportedKeySystemVersion requests
 *
 * Sequence Diagrams:
 *   1: Check Supported Key Systems - indicative use of Rialto
 *   2: Get DRM Version - Netflix/native Rialto
 *   From: https://wiki.rdkcentral.com/pages/viewpage.action?pageId=226375556
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeysCapabilities with stubs for RialtoClient and ocdm
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts rialtoServer running in its own thread
 *
 * Test Steps:
 *  Step C1: Get all the supported key systems
 *   Expect that getSupportedKeySystems is propagated to the server from the client
 *   The server will repeatedly call OCDM isTypeSupported() for every
 *     key system that rialto supports
 *   Check the array of key systems returned to the client
 *
 *  Step C2: Check if key type is supported with yes response
 *   Expect that supportsKeySystem is propagated to server from client
 *   The server should call OCDM isTypeSupported() for this key type
 *     and OCDM will return yes
 *   The server will respond to the client and indicate that it is supported
 *
 *  Step C3: Check if key is supported with no response
 *   Expect that supportsKeySystem is propagated to server from client
 *   The server should call OCDM isTypeSupported() for this key type
 *     and OCDM will return NO
 *   The server will respond to the client and indicate that it is NOT supported
 *
 *  Step C4: Get the supported key system version
 *   Expect that getSupportedKeySystemVersion is propagated to server.
 *   The server should call createOcdmSystem() on the OCDM system factory for this key system
 *   The server should call OCDM getVersion() to obtain the version number string
 *   The server should respond to the client with the version number
 *
 *  Step C5: Get the supported key system version for a different system
 *   Expect that getSupportedKeySystemVersion is propagated to server.
 *   The server should call createOcdmSystem() on the OCDM system factory for this key system
 *     and OCDM will return a different OCDM system to test C4
 *   The server should call OCDM getVersion() to obtain the version number string
 *   The server should respond to the client with the version number
 *   EXTRA test not in C4... the server will destroy the OCDM system
 *
 *  Step C6: Get the supported key system version - failure
 *   Expect that getSupportedKeySystemVersion is propagated to server.
 *   The server should call createOcdmSystem() on the OCDM system factory for this key system
 *   The server should call OCDM getVersion() but it returns failure
 *   The server should respond to the client with the failure indication
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *    All of the detailed steps outlined above should be performed
 *
 * Code:
 */
TEST_F(MediaKeysCapabilitiesTest, philTest)
{
    // Step C1: Get all the supported key systems
    willGetSupportedKeySystemsWithAllSupported();
    getSupportedKeySystemsWithAllSupported();

    // Step C2: Check if key type is supported with yes response
    willCallSupportsKeySystem();
    callSupportsKeySystem();

    // Step C3: Check if key is supported with no response
    willCallSupportKeySystemWithResponseNo();
    callSupportKeySystemWithResponseNo();

    // Step C4: Get the supported key system version
    willGetSupportedKeySystemVersionRequest1();
    getSupportedKeySystemVersionRequest1();

    // Step C5: Get the supported key system version for a different system
    willGetSupportedKeySystemVersionRequest2();
    getSupportedKeySystemVersionRequest2();

    // Step C6: Get the supported key system version - failure
    willGetSupportedKeySystemVersionRequestFail();
    getSupportedKeySystemVersionRequestFail();
}
} // namespace firebolt::rialto::server::ct
