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

class RialtoServerMediaKeySessionLoadSessionTest : public MediaKeySessionTestBase
{
protected:
    RialtoServerMediaKeySessionLoadSessionTest() { createKeySession(kNetflixKeySystem); }
};

/**
 * Test that LoadSession can load successfully.
 */
TEST_F(RialtoServerMediaKeySessionLoadSessionTest, Success)
{
    EXPECT_CALL(*m_ocdmSessionMock, load()).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->loadSession());
}

/**
 * Test that LoadSession fails if the ocdm session api fails.
 */
TEST_F(RialtoServerMediaKeySessionLoadSessionTest, OcdmSessionFailure)
{
    EXPECT_CALL(*m_ocdmSessionMock, load()).WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));
    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED, m_mediaKeySession->loadSession());
}
