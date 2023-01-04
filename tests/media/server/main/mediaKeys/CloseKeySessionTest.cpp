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

class RialtoServerMediaKeysCloseKeySessionTest : public MediaKeysTestBase
{
protected:
    RialtoServerMediaKeysCloseKeySessionTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
    ~RialtoServerMediaKeysCloseKeySessionTest() { destroyMediaKeys(); }
};

/**
 * Test that CloseKeySession can close successfully.
 */
TEST_F(RialtoServerMediaKeysCloseKeySessionTest, Success)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, closeKeySession()).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->closeKeySession(m_kKeySessionId));
}

/**
 * Test that CloseKeySession fails if the key session does not exsist.
 */
TEST_F(RialtoServerMediaKeysCloseKeySessionTest, SessionDoesNotExistFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID, m_mediaKeys->closeKeySession(m_kKeySessionId + 1));
}

/**
 * Test that CloseKeySession fails if the session api fails.
 */
TEST_F(RialtoServerMediaKeysCloseKeySessionTest, SessionFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, closeKeySession()).WillOnce(Return(MediaKeyErrorStatus::INVALID_STATE));

    EXPECT_EQ(MediaKeyErrorStatus::INVALID_STATE, m_mediaKeys->closeKeySession(m_kKeySessionId));
}

/**
 * Test that CloseKeySession key close session is deferred when some buffers use it
 */
TEST_F(RialtoServerMediaKeysCloseKeySessionTest, SessionNotClosedWhenBuffersUsed)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->closeKeySession(m_kKeySessionId));
}

/**
 * Decrement closes the session when no buffers use it
 */
TEST_F(RialtoServerMediaKeysCloseKeySessionTest, SessionClosedAfterDecrement)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->closeKeySession(m_kKeySessionId));

    EXPECT_CALL(*m_mediaKeySessionMock, closeKeySession()).WillOnce(Return(MediaKeyErrorStatus::OK));
    m_mediaKeys->decrementSessionIdUsageCounter(m_kKeySessionId);
}

/**
 * Decrement doesn't close the session when some buffers use it
 */
TEST_F(RialtoServerMediaKeysCloseKeySessionTest, SessionNotClosedAfterDecrementWhenBuffersStillInUse)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->closeKeySession(m_kKeySessionId));

    m_mediaKeys->decrementSessionIdUsageCounter(m_kKeySessionId);
}

/**
 * Decrement doesn't close the session when close wasn't requested
 */
TEST_F(RialtoServerMediaKeysCloseKeySessionTest, SessionNotClosedAfterDecrementWhenCloseNotRequested)
{
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->decrementSessionIdUsageCounter(m_kKeySessionId);
}

/**
 * Test that CloseKeySession can close when increment fails
 */
TEST_F(RialtoServerMediaKeysCloseKeySessionTest, SessionClosedWhenIncrementFails)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId + 1);
    EXPECT_CALL(*m_mediaKeySessionMock, closeKeySession()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->closeKeySession(m_kKeySessionId));
}

/**
 * Test that CloseKeySession can close when decrement fails
 */
TEST_F(RialtoServerMediaKeysCloseKeySessionTest, SessionNotClosedWhenDeccrementFails)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    m_mediaKeys->decrementSessionIdUsageCounter(m_kKeySessionId + 1);
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->closeKeySession(m_kKeySessionId));
}