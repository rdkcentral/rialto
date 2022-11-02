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

class RialtoServerMediaKeySessionGenerateRequestTest : public MediaKeySessionTestBase
{
protected:
    InitDataType m_initDataType = InitDataType::CENC;
    std::vector<uint8_t> m_initData{1, 2, 3};

    RialtoServerMediaKeySessionGenerateRequestTest() { createKeySession(kNetflixKeySystem); }
};

/**
 * Test that GenerateRequest can generate request successfully.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, Success)
{
    EXPECT_CALL(*m_ocdmSessionMock, constructSession(m_keySessionType, m_initDataType, &m_initData[0], m_initData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_initDataType, m_initData));

    // Close ocdm before destroying
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that GenerateRequest fails if session already constructed.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, SessionAlreadyConstructedFailure)
{
    EXPECT_CALL(*m_ocdmSessionMock, constructSession(m_keySessionType, m_initDataType, &m_initData[0], m_initData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeySession->generateRequest(m_initDataType, m_initData));

    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_mediaKeySession->generateRequest(m_initDataType, m_initData));

    // OcdmSession will be closed on destruction
    expectCloseKeySession(kNetflixKeySystem);
}

/**
 * Test that GenerateRequest fails if the ocdm session api fails.
 */
TEST_F(RialtoServerMediaKeySessionGenerateRequestTest, OcdmSessionFailure)
{
    EXPECT_CALL(*m_ocdmSessionMock, constructSession(m_keySessionType, m_initDataType, &m_initData[0], m_initData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));
    EXPECT_EQ(MediaKeyErrorStatus::NOT_SUPPORTED, m_mediaKeySession->generateRequest(m_initDataType, m_initData));
}
