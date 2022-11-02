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

class RialtoServerMediaKeysCreateKeySessionTest : public MediaKeysTestBase
{
protected:
    RialtoServerMediaKeysCreateKeySessionTest() { createMediaKeys(kNetflixKeySystem); }
};

/**
 * Test that CreateKeySession can create a session successfully and return a valid key id.
 */
TEST_F(RialtoServerMediaKeysCreateKeySessionTest, Success)
{
    int32_t returnKeySessionId = -1;
    EXPECT_CALL(*m_mediaKeySessionFactoryMock,
                createMediaKeySession(kNetflixKeySystem, _, _, m_keySessionType, _, m_isLDL))
        .WillOnce(Return(ByMove(std::move(m_mediaKeySession))));

    EXPECT_EQ(MediaKeyErrorStatus::OK,
              m_mediaKeys->createKeySession(m_keySessionType, m_mediaKeysClientMock, m_isLDL, returnKeySessionId));
    EXPECT_GE(returnKeySessionId, -1);
    EXPECT_TRUE(m_mediaKeys->hasSession(returnKeySessionId));
}

/**
 * Test that CreateKeySession fails if ocdm creation fails.
 */
TEST_F(RialtoServerMediaKeysCreateKeySessionTest, OcdmSystemFailure)
{
    int32_t returnKeySessionId = -1;
    EXPECT_CALL(*m_mediaKeySessionFactoryMock,
                createMediaKeySession(kNetflixKeySystem, _, _, m_keySessionType, _, m_isLDL))
        .WillOnce(Return(ByMove(nullptr)));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL,
              m_mediaKeys->createKeySession(m_keySessionType, m_mediaKeysClientMock, m_isLDL, returnKeySessionId));
}
