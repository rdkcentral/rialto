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
class DrmStoreTest : public MediaKeysTestMethods
{
public:
    DrmStoreTest() {}
    virtual ~DrmStoreTest() {}

    void willDeleteDrmStoreRequest();
    void deleteDrmStoreRequest();

    void willGetDrmStoreHashRequest();
    void getDrmStoreHashRequest();

    void willGetDrmStoreHashRequestFails();
    void getDrmStoreHashRequestFails();

    const std::vector<unsigned char> m_kHashTest{'d', 'z', 'f'};
};

void DrmStoreTest::willDeleteDrmStoreRequest()
{
    EXPECT_CALL(*m_ocdmSystemMock, deleteSecureStore()).WillOnce(Return(MediaKeyErrorStatus::OK));
}
void DrmStoreTest::deleteDrmStoreRequest()
{
    auto request{createDeleteDrmStoreRequest(m_mediaKeysHandle)};

    ConfigureAction<DeleteDrmStore>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::DeleteDrmStoreResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void DrmStoreTest::willGetDrmStoreHashRequest()
{
    EXPECT_CALL(*m_ocdmSystemMock, getSecureStoreHash(_, _))
        .WillOnce(testing::Invoke(
            [&](uint8_t secureStoreHash[], uint32_t secureStoreHashLength) -> MediaKeyErrorStatus
            {
                // The real wrapper calls opencdm_get_secure_store_hash_ext()
                // defined in opencdm/opencdm_ext.h
                // and this header specifies the length should be at least 64
                // (but doesn't return the number of bytes actually filled)
                EXPECT_GT(secureStoreHashLength, 64);
                size_t i = 0;
                for (; i < m_kHashTest.size(); ++i)
                {
                    secureStoreHash[i] = m_kHashTest[i];
                }
                // Pad with zeros in case valgrind complains...
                for (; i < secureStoreHashLength; ++i)
                {
                    secureStoreHash[i] = 0;
                }
                return MediaKeyErrorStatus::OK;
            }));
}
void DrmStoreTest::getDrmStoreHashRequest()
{
    auto request{createGetDrmStoreHashRequest(m_mediaKeysHandle)};

    ConfigureAction<GetDrmStoreHash>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse(
            [&](const firebolt::rialto::GetDrmStoreHashResponse &resp)
            {
                EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK);
                for (size_t i = 0; i < m_kHashTest.size(); ++i)
                {
                    EXPECT_EQ(resp.drm_store_hash(i), m_kHashTest[i]);
                }
            });
}

void DrmStoreTest::willGetDrmStoreHashRequestFails()
{
    EXPECT_CALL(*m_ocdmSystemMock, getSecureStoreHash(_, _))
        .WillOnce(testing::Invoke([&](uint8_t secureStoreHash[], uint32_t secureStoreHashLength) -> MediaKeyErrorStatus
                                  { return MediaKeyErrorStatus::NOT_SUPPORTED; }));
}
void DrmStoreTest::getDrmStoreHashRequestFails()
{
    auto request{createGetDrmStoreHashRequest(m_mediaKeysHandle)};

    ConfigureAction<GetDrmStoreHash>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GetDrmStoreHashResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::NOT_SUPPORTED); });
}

/*
 * Component Test: Drm Store APIs.
 * Test Objective:
 *  Test the deleteDrmStore and getDrmStoreHash APIs.
 *
 * Sequence Diagrams:
 *  Delete DRM Store, Get DRM Store Hash
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
 *  Step 1: Get the drm store
 *   Expect that getDrmStoreHash is processed by the server.
 *   Api call returns with success
 *   Check drm store hash.
 *
 *  Step 2: Get the drm store failure
 *   Expect that getDrmStoreHash is processed by the server.
 *   Api call returns with failure.
 *
 *  Step 3: Delete the drm store
 *   Expect that deleteDrmStore is processed by the server.
 *   Api call returns with success.
 *
 * Test Teardown:
 *  Destroy MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get and delete the drm store successfully.
 *
 * Code:
 */
TEST_F(DrmStoreTest, shouldDrmstore)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Get the drm store
    willGetDrmStoreHashRequest();
    getDrmStoreHashRequest();

    // Step 2: Get the drm store failure
    willGetDrmStoreHashRequestFails();
    getDrmStoreHashRequestFails();

    // Step 3: Delete the drm store
    willDeleteDrmStoreRequest();
    deleteDrmStoreRequest();
}

} // namespace firebolt::rialto::server::ct
