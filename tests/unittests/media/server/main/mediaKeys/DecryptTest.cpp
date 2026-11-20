/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "MediaKeysTestBase.h"

class RialtoServerMediaKeysDecryptTest : public MediaKeysTestBase
{
protected:
    GstBuffer m_encrypted{};
    GstCaps m_caps{};

    RialtoServerMediaKeysDecryptTest()
    {
        createMediaKeys(kWidevineKeySystem);
        createKeySession(kWidevineKeySystem);
    }
    ~RialtoServerMediaKeysDecryptTest() { destroyMediaKeys(); }
};

/**
 * Test that Decryp can update successfully.
 */
TEST_F(RialtoServerMediaKeysDecryptTest, Success)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, decrypt(&m_encrypted, &m_caps)).WillOnce(Return(wrappers::DecryptionStatus::OK));

    EXPECT_EQ(wrappers::DecryptionStatus::OK, m_mediaKeys->decrypt(m_kKeySessionId, &m_encrypted, &m_caps));
}

/**
 * Test that Decryp fails if the key session does not exsist.
 */
TEST_F(RialtoServerMediaKeysDecryptTest, SessionDoesNotExistFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(wrappers::DecryptionStatus::INVALID_SESSION,
              m_mediaKeys->decrypt(m_kKeySessionId + 1, &m_encrypted, &m_caps));
}

/**
 * Test that Decryp fails if the session api fails.
 */
TEST_F(RialtoServerMediaKeysDecryptTest, DecryptFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, decrypt(&m_encrypted, &m_caps)).WillOnce(Return(wrappers::DecryptionStatus::FAIL));

    EXPECT_EQ(wrappers::DecryptionStatus::FAIL, m_mediaKeys->decrypt(m_kKeySessionId, &m_encrypted, &m_caps));
}
