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

class RialtoServerMediaKeysSetDrmHeaderTest : public MediaKeysTestBase
{
protected:
    const std::vector<uint8_t> m_kDrmHeader{4, 4, 5};
    RialtoServerMediaKeysSetDrmHeaderTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
    ~RialtoServerMediaKeysSetDrmHeaderTest() { destroyMediaKeys(); }
};

/**
 * Test that setDrmHeader can load successfully.
 */
TEST_F(RialtoServerMediaKeysSetDrmHeaderTest, Success)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, setDrmHeader(m_kDrmHeader)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->setDrmHeader(m_kKeySessionId, m_kDrmHeader));
}

/**
 * Test that setDrmHeader fails if the key session does not exsist.
 */
TEST_F(RialtoServerMediaKeysSetDrmHeaderTest, SessionDoesNotExistFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID, m_mediaKeys->setDrmHeader(m_kKeySessionId + 1, m_kDrmHeader));
}

/**
 * Test that setDrmHeader fails if the session api fails.
 */
TEST_F(RialtoServerMediaKeysSetDrmHeaderTest, SetDrmHeaderFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, setDrmHeader(m_kDrmHeader)).WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));

    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED, m_mediaKeys->setDrmHeader(m_kKeySessionId, m_kDrmHeader));
}
