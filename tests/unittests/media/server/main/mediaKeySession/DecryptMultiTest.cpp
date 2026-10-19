/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
 * Copyright 2026 Liberty Global Service B.V.
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

using testing::DoAll;
using testing::SetArgReferee;

class RialtoServerMediaKeySessionDecryptMultiTest : public MediaKeySessionTestBase
{
protected:
    GstBuffer m_encrypted1{};
    GstBuffer m_encrypted2{};
    std::vector<GstBuffer *> m_bufferVec{&m_encrypted1, &m_encrypted2};
    GstCaps m_caps{};

    ~RialtoServerMediaKeySessionDecryptMultiTest() { destroyKeySession(); }
};

/**
 * Test that buffers can be decrypted successfully
 */
TEST_F(RialtoServerMediaKeySessionDecryptMultiTest, Success)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, decryptBufferMulti(m_bufferVec, &m_caps)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->decryptMulti(m_bufferVec, &m_caps));
}

/**
 * Test that method returns failure when decryption fails
 */
TEST_F(RialtoServerMediaKeySessionDecryptMultiTest, OcdmSessionFailure)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, decryptBufferMulti(m_bufferVec, &m_caps)).WillOnce(Return(MediaKeyErrorStatus::FAIL));
    EXPECT_CALL(*m_ocdmSessionMock, getLastDrmError(_)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->decryptMulti(m_bufferVec, &m_caps));
}

/**
 * Test that an HDCP output protection DRM error is translated to OUTPUT_RESTRICTED.
 */
TEST_F(RialtoServerMediaKeySessionDecryptMultiTest, OcdmSessionOutputRestricted)
{
    constexpr uint32_t kHdcpOutputProtectionFailure{4427};

    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, decryptBufferMulti(m_bufferVec, &m_caps)).WillOnce(Return(MediaKeyErrorStatus::FAIL));
    EXPECT_CALL(*m_ocdmSessionMock, getLastDrmError(_))
        .WillOnce(DoAll(SetArgReferee<0>(kHdcpOutputProtectionFailure), Return(MediaKeyErrorStatus::OK)));

    EXPECT_EQ(MediaKeyErrorStatus::OUTPUT_RESTRICTED, m_mediaKeySession->decryptMulti(m_bufferVec, &m_caps));
}

/**
 * Test that OUTPUT_RESTRICTED is propagated when returned directly by decryptBufferMulti.
 */
TEST_F(RialtoServerMediaKeySessionDecryptMultiTest, OcdmSessionDirectOutputRestricted)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, decryptBufferMulti(m_bufferVec, &m_caps))
        .WillOnce(Return(MediaKeyErrorStatus::OUTPUT_RESTRICTED));

    EXPECT_EQ(MediaKeyErrorStatus::OUTPUT_RESTRICTED, m_mediaKeySession->decryptMulti(m_bufferVec, &m_caps));
}

/**
 * Test that decryptMulti fails if ocdm onError is called during the operation.
 */
TEST_F(RialtoServerMediaKeySessionDecryptMultiTest, OnErrorFailure)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, decryptBufferMulti(m_bufferVec, &m_caps))
        .WillOnce(Invoke(
            [this](std::vector<GstBuffer *> &bufferVec, GstCaps *caps)
            {
                m_mediaKeySession->onError("Failure");
                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->decryptMulti(m_bufferVec, &m_caps));
}
