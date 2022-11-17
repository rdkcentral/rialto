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

class RialtoServerMediaKeySessionGetLastDrmErrorTest : public MediaKeySessionTestBase
{
protected:
    uint32_t m_lastDrmError{0};
    ~RialtoServerMediaKeySessionGetLastDrmErrorTest() { destroyKeySession(); }
};

/**
 * Test that last drm error can be get successfully
 */
TEST_F(RialtoServerMediaKeySessionGetLastDrmErrorTest, Success)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, getLastDrmError(m_lastDrmError)).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->getLastDrmError(m_lastDrmError));
}

/**
 * Test that method returns failure when get last drm error fails
 */
TEST_F(RialtoServerMediaKeySessionGetLastDrmErrorTest, Fail)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, getLastDrmError(m_lastDrmError)).WillOnce(Return(MediaKeyErrorStatus::FAIL));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->getLastDrmError(m_lastDrmError));
}
