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

class RialtoServerMediaKeysGetChallengeDataTest : public MediaKeysTestBase
{
protected:
    RialtoServerMediaKeysGetChallengeDataTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
};

/**
 * Test that get challenge data is called.
 */
TEST_F(RialtoServerMediaKeysGetChallengeDataTest, Success)
{
    EXPECT_CALL(*m_mediaKeySessionMock, getChallengeData());
    m_mediaKeys->getChallengeData(m_keySessionId);
}

/**
 * Test that get challenge data is not called when session is not present.
 */
TEST_F(RialtoServerMediaKeysGetChallengeDataTest, SessionDoesNotExistFailure)
{
    m_mediaKeys->getChallengeData(m_keySessionId + 1);
}
