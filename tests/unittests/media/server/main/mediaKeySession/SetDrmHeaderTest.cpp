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

MATCHER_P(drmHeaderMatcher, header, "")
{
    for (size_t i = 0; i < header.size(); ++i)
    {
        if (arg[i] != header[i])
        {
            return false;
        }
    }
    return true;
}

class RialtoServerMediaKeySessionSetDrmHeaderTest : public MediaKeySessionTestBase
{
protected:
    const std::vector<uint8_t> m_kDrmHeader{3, 2, 1};
    ~RialtoServerMediaKeySessionSetDrmHeaderTest() { destroyKeySession(); }
};

/**
 * Test that drm header can be set successfully
 */
TEST_F(RialtoServerMediaKeySessionSetDrmHeaderTest, Success)
{
    createKeySession(kNetflixKeySystem);

    generateRequestPlayready();

    EXPECT_CALL(*m_ocdmSessionMock, setDrmHeader(&m_kDrmHeader[0], m_kDrmHeader.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->setDrmHeader(m_kDrmHeader));

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that setDrmHeader fails if the ocdm session api set drm header fails.
 */
TEST_F(RialtoServerMediaKeySessionSetDrmHeaderTest, OcdmSessionFailure)
{
    createKeySession(kNetflixKeySystem);

    generateRequestPlayready();

    EXPECT_CALL(*m_ocdmSessionMock, setDrmHeader(&m_kDrmHeader[0], m_kDrmHeader.size()))
        .WillOnce(Return(MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->setDrmHeader(m_kDrmHeader));

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that setDrmHeader fails if the ocdm onError is called during the operation.
 */
TEST_F(RialtoServerMediaKeySessionSetDrmHeaderTest, OnErrorFailure)
{
    createKeySession(kNetflixKeySystem);

    generateRequestPlayready();

    EXPECT_CALL(*m_ocdmSessionMock, setDrmHeader(&m_kDrmHeader[0], m_kDrmHeader.size()))
        .WillOnce(Invoke(
            [this](const uint8_t drmHeader[], uint32_t drmHeaderSize)
            {
                m_mediaKeySession->onError("Failure");
                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->setDrmHeader(m_kDrmHeader));

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that drm header can be set successfully after being queued before session is constructed
 */
TEST_F(RialtoServerMediaKeySessionSetDrmHeaderTest, SuccessQueued)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->setDrmHeader(m_kDrmHeader));

    EXPECT_CALL(*m_ocdmSessionMock, setDrmHeader(drmHeaderMatcher(m_kDrmHeader), m_kDrmHeader.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    generateRequestPlayready();

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}
