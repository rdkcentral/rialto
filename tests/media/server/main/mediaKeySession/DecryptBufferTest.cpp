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

#include "MediaKeySessionTestBase.h"

#ifdef RIALTO_ENABLE_DECRYPT_BUFFER

class RialtoServerMediaKeySessionDecryptBufferTest : public MediaKeySessionTestBase
{
protected:
    GstBuffer m_encrypted{};
    GstCaps m_caps{};

    ~RialtoServerMediaKeySessionDecryptBufferTest() { destroyKeySession(); }
};

/**
 * Test that buffer can be decrypted successfully
 */
TEST_F(RialtoServerMediaKeySessionDecryptBufferTest, Success)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, decryptBuffer(&m_encrypted, &m_caps)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->decrypt(&m_encrypted, &m_caps));
}

/**
 * Test that method returns failure when decryption fails
 */
TEST_F(RialtoServerMediaKeySessionDecryptBufferTest, OcdmSessionFailure)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, decryptBuffer(&m_encrypted, &m_caps)).WillOnce(Return(MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->decrypt(&m_encrypted, &m_caps));
}

/**
 * Test that GetCdmKeySessionId fails if ocdm onError is called during the operation.
 */
TEST_F(RialtoServerMediaKeySessionDecryptBufferTest, OnErrorFailure)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, decryptBuffer(&m_encrypted, &m_caps))
        .WillOnce(Invoke([this](GstBuffer * encrypted, GstCaps *caps)
            {
                m_mediaKeySession->onError("Failure");
                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->decrypt(&m_encrypted, &m_caps));
}
#endif
