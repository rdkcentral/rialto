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

class RialtoServerMediaKeySessionDecryptTest : public MediaKeySessionTestBase
{
protected:
    GstBuffer m_encrypted{};
    GstBuffer m_subSample{};
    const uint32_t m_kSubSampleCount{2};
    GstBuffer m_IV{};
    GstBuffer m_keyId{};
    uint32_t m_initWithLast15{1};

    ~RialtoServerMediaKeySessionDecryptTest() { destroyKeySession(); }
};

/**
 * Test that buffer can be decrypted successfully
 */
TEST_F(RialtoServerMediaKeySessionDecryptTest, Success)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock,
                decrypt(&m_encrypted, &m_subSample, m_kSubSampleCount, &m_IV, &m_keyId, m_initWithLast15))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->decrypt(&m_encrypted, &m_subSample, m_kSubSampleCount, &m_IV,
                                                                  &m_keyId, m_initWithLast15));
}

/**
 * Test that method returns failure when decryption fails
 */
TEST_F(RialtoServerMediaKeySessionDecryptTest, Fail)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock,
                decrypt(&m_encrypted, &m_subSample, m_kSubSampleCount, &m_IV, &m_keyId, m_initWithLast15))
        .WillOnce(Return(MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->decrypt(&m_encrypted, &m_subSample, m_kSubSampleCount,
                                                                    &m_IV, &m_keyId, m_initWithLast15));
}
