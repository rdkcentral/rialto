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

class RialtoServerMediaKeySessionSelectKeyIdTest : public MediaKeySessionTestBase
{
protected:
    const std::vector<uint8_t> m_kKeyId{1, 2, 3};
    ~RialtoServerMediaKeySessionSelectKeyIdTest() { destroyKeySession(); }
};

/**
 * Test that selectKeyId executes successfully
 */
TEST_F(RialtoServerMediaKeySessionSelectKeyIdTest, Success)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, selectKeyId(m_kKeyId.size(), m_kKeyId.data())).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->selectKeyId(m_kKeyId));
}

/**
 * Test that selectKeyId does not select the same key twice
 */
TEST_F(RialtoServerMediaKeySessionSelectKeyIdTest, DoNotSetTheSameKeyTwice)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, selectKeyId(m_kKeyId.size(), m_kKeyId.data()))
        .Times(1)
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->selectKeyId(m_kKeyId));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->selectKeyId(m_kKeyId));
}

/**
 * Test that method returns failure when decryption fails
 */
TEST_F(RialtoServerMediaKeySessionSelectKeyIdTest, Fail)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, selectKeyId(m_kKeyId.size(), m_kKeyId.data()))
        .WillOnce(Return(MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->selectKeyId(m_kKeyId));
}

/**
 * Test that selected key is saved only if operation succeeds
 */
TEST_F(RialtoServerMediaKeySessionSelectKeyIdTest, SaveKeyAfterSuccessfulOperationOnly)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, selectKeyId(m_kKeyId.size(), m_kKeyId.data()))
        .WillOnce(Return(MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->selectKeyId(m_kKeyId));

    EXPECT_CALL(*m_ocdmSessionMock, selectKeyId(m_kKeyId.size(), m_kKeyId.data()))
        .Times(1)
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->selectKeyId(m_kKeyId));
}

/**
 * Test that selectKeyId fails if ocdm onError is called during the operation.
 */
TEST_F(RialtoServerMediaKeySessionSelectKeyIdTest, OnErrorFailure)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, selectKeyId(m_kKeyId.size(), m_kKeyId.data()))
        .WillOnce(Invoke([this](uint8_t keyLength, const uint8_t keyId[])
            {
                m_mediaKeySession->onError("Failure");
                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->selectKeyId(m_kKeyId));
}
