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

class RialtoServerCreateMediaKeySessionTest : public MediaKeySessionTestBase
{
};

/**
 * Test that a MediaKeySession object can be created successfully.
 */
TEST_F(RialtoServerCreateMediaKeySessionTest, Create)
{
    EXPECT_CALL(*m_ocdmSystemMock, createSession(_)).WillOnce(Return(ByMove(std::move(m_ocdmSession))));

    EXPECT_NO_THROW(m_mediaKeySession = std::make_unique<MediaKeySession>(kNetflixKeySystem, m_keySessionId,
                                                                          *m_ocdmSystemMock, m_keySessionType,
                                                                          m_mediaKeysClientMock, m_isLDL));
    EXPECT_NE(m_mediaKeySession, nullptr);
}

/**
 * Test that a MediaKeySession object throws an exeption if failure occurs during construction.
 * In this case, createOcdmSession fails, returning a nullptr.
 */
TEST_F(RialtoServerCreateMediaKeySessionTest, CreateOcdmSessionFailure)
{
    EXPECT_CALL(*m_ocdmSystemMock, createSession(_)).WillOnce(Return(ByMove(std::move(nullptr))));

    EXPECT_THROW(m_mediaKeySession = std::make_unique<MediaKeySession>(kNetflixKeySystem, m_keySessionId,
                                                                       *m_ocdmSystemMock, m_keySessionType,
                                                                       m_mediaKeysClientMock, m_isLDL),
                 std::runtime_error);
}
