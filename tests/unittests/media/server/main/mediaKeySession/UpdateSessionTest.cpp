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

class RialtoServerMediaKeySessionUpdateSessionTest : public MediaKeySessionTestBase
{
protected:
    const std::vector<uint8_t> m_kResponseData{1, 2, 3};

    ~RialtoServerMediaKeySessionUpdateSessionTest() { destroyKeySession(); }
};

/**
 * Test that UpdateSession can update successfully for netflix key system.
 */
TEST_F(RialtoServerMediaKeySessionUpdateSessionTest, SuccessNetflix)
{
    createKeySession(kNetflixKeySystem);
    generateRequestPlayready();

    EXPECT_CALL(*m_ocdmSessionMock, storeLicenseData(&m_kResponseData[0], m_kResponseData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->updateSession(m_kResponseData));

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that UpdateSession can update successfully for none netflix key systems.
 */
TEST_F(RialtoServerMediaKeySessionUpdateSessionTest, SuccessNoneNetflix)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, update(&m_kResponseData[0], m_kResponseData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->updateSession(m_kResponseData));
}

/**
 * Test that UpdateSession fails if the ocdm session api StoreLicenseData fails.
 */
TEST_F(RialtoServerMediaKeySessionUpdateSessionTest, OcdmSessionStoreLicenseDataFailure)
{
    createKeySession(kNetflixKeySystem);
    generateRequestPlayready();

    EXPECT_CALL(*m_ocdmSessionMock, storeLicenseData(&m_kResponseData[0], m_kResponseData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::INVALID_STATE));

    EXPECT_EQ(MediaKeyErrorStatus::INVALID_STATE, m_mediaKeySession->updateSession(m_kResponseData));

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that UpdateSession fails if the ocdm session api Update fails.
 */
TEST_F(RialtoServerMediaKeySessionUpdateSessionTest, OcdmSessionUpdateFailure)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, update(&m_kResponseData[0], m_kResponseData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::INVALID_STATE));

    EXPECT_EQ(MediaKeyErrorStatus::INVALID_STATE, m_mediaKeySession->updateSession(m_kResponseData));
}

/**
 * Test that UpdateSession fails if the ocdm onError is called during the operation.
 */
TEST_F(RialtoServerMediaKeySessionUpdateSessionTest, OcdmSessionUpdateOnErrorFailure)
{
    createKeySession(kWidevineKeySystem);

    EXPECT_CALL(*m_ocdmSessionMock, update(&m_kResponseData[0], m_kResponseData.size()))
        .WillOnce(Invoke(
            [this](const uint8_t response[], uint32_t responseSize)
            {
                m_mediaKeySession->onError("Failure");
                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->updateSession(m_kResponseData));
}
