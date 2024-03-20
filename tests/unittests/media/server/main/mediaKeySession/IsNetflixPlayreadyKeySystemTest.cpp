/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

class RialtoServerMediaKeySessionIsNetflixPlayreadyKeySystemTest : public MediaKeySessionTestBase
{
protected:
    ~RialtoServerMediaKeySessionIsNetflixPlayreadyKeySystemTest() { destroyKeySession(); }
};

/**
 * Test that isNetflixPlayreadyKeySystem returns false for microsoft playready key system
 */
TEST_F(RialtoServerMediaKeySessionIsNetflixPlayreadyKeySystemTest, ReturnFalseForMsPlayready)
{
    createKeySession(kPlayreadyKeySystem);

    EXPECT_FALSE(m_mediaKeySession->isNetflixPlayreadyKeySystem());
}

/**
 * Test that isNetflixPlayreadyKeySystem returns true for netflix key system
 */
TEST_F(RialtoServerMediaKeySessionIsNetflixPlayreadyKeySystemTest, ReturnTrueForNetflix)
{
    createKeySession(kNetflixKeySystem);

    EXPECT_TRUE(m_mediaKeySession->isNetflixPlayreadyKeySystem());
}

/**
 * Test that isNetflixPlayreadyKeySystem returns false for widevine key system
 */
TEST_F(RialtoServerMediaKeySessionIsNetflixPlayreadyKeySystemTest, ReturnFalseForWidevine)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_FALSE(m_mediaKeySession->isNetflixPlayreadyKeySystem());
}
