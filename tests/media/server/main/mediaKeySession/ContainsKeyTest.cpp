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

class RialtoServerMediaKeySessionContainsKeyTest : public MediaKeySessionTestBase
{
protected:
    const std::vector<uint8_t> m_kKeyId{1, 2, 3};
};

/**
 * Test that function returns true if key is found
 */
TEST_F(RialtoServerMediaKeySessionContainsKeyTest, ReturnTrue)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, hasKeyId(&m_kKeyId[0], m_kKeyId.size())).WillOnce(Return(1));

    EXPECT_TRUE(m_mediaKeySession->containsKey(m_kKeyId));
}

/**
 * Test that function returns false if key is not found
 */
TEST_F(RialtoServerMediaKeySessionContainsKeyTest, ReturnFalse)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, hasKeyId(&m_kKeyId[0], m_kKeyId.size())).WillOnce(Return(0));

    EXPECT_FALSE(m_mediaKeySession->containsKey(m_kKeyId));
}
