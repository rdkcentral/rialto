/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

class RialtoServerMediaKeysReleaseKeySessionTest : public MediaKeysTestBase
{
protected:
    RialtoServerMediaKeysReleaseKeySessionTest()
    {
        createMediaKeys(kNetflixKeySystem);
        createKeySession(kNetflixKeySystem);
    }
    ~RialtoServerMediaKeysReleaseKeySessionTest() { destroyMediaKeys(); }
};

/**
 * Test that ReleaseKeySession can release successfully.
 */
TEST_F(RialtoServerMediaKeysReleaseKeySessionTest, Success)
{
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->releaseKeySession(m_kKeySessionId));

    // Session is removed, so any function call should return BAD_SESSION_ID now:
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID, m_mediaKeys->removeKeySession(m_kKeySessionId));
}

/**
 * Test that ReleaseKeySession fails if the key session does not exsist.
 */
TEST_F(RialtoServerMediaKeysReleaseKeySessionTest, SessionDoesNotExistFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID, m_mediaKeys->releaseKeySession(m_kKeySessionId + 1));

    // Session should still exist, so any function call should be executed:
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, removeKeySession()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->removeKeySession(m_kKeySessionId));
}

/**
 * Test that ReleaseKeySession key release session is deferred when some buffers use it
 */
TEST_F(RialtoServerMediaKeysReleaseKeySessionTest, SessionNotReleasedWhenBuffersUsed)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->releaseKeySession(m_kKeySessionId));

    // Session should still exist, so any function call should be executed:
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, removeKeySession()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->removeKeySession(m_kKeySessionId));
}

/**
 * Decrement releases the session when no buffers use it
 */
TEST_F(RialtoServerMediaKeysReleaseKeySessionTest, SessionReleasedAfterDecrement)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->releaseKeySession(m_kKeySessionId));

    m_mediaKeys->decrementSessionIdUsageCounter(m_kKeySessionId);

    // Session is removed, so any function call should return BAD_SESSION_ID now:
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID, m_mediaKeys->removeKeySession(m_kKeySessionId));
}

/**
 * Decrement doesn't release the session when some buffers use it
 */
TEST_F(RialtoServerMediaKeysReleaseKeySessionTest, SessionNotReleasedAfterDecrementWhenBuffersStillInUse)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->releaseKeySession(m_kKeySessionId));

    m_mediaKeys->decrementSessionIdUsageCounter(m_kKeySessionId);

    // Session should still exist, so any function call should be executed:
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, removeKeySession()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->removeKeySession(m_kKeySessionId));
}

/**
 * Decrement doesn't release the session when release wasn't requested
 */
TEST_F(RialtoServerMediaKeysReleaseKeySessionTest, SessionNotReleasedAfterDecrementWhenReleaseNotRequested)
{
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->decrementSessionIdUsageCounter(m_kKeySessionId);

    // Session should still exist, so any function call should be executed:
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, removeKeySession()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->removeKeySession(m_kKeySessionId));
}

/**
 * Test that ReleaseKeySession can release when increment fails
 */
TEST_F(RialtoServerMediaKeysReleaseKeySessionTest, SessionReleasedWhenIncrementFails)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId + 1);
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->releaseKeySession(m_kKeySessionId));

    // Session is removed, so any function call should return BAD_SESSION_ID now:
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(MediaKeyErrorStatus::BAD_SESSION_ID, m_mediaKeys->removeKeySession(m_kKeySessionId));
}

/**
 * Test that ReleaseKeySession can release when decrement fails
 */
TEST_F(RialtoServerMediaKeysReleaseKeySessionTest, SessionNotReleasedWhenDeccrementFails)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTaskAndWait();

    m_mediaKeys->incrementSessionIdUsageCounter(m_kKeySessionId);
    m_mediaKeys->decrementSessionIdUsageCounter(m_kKeySessionId + 1);
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->releaseKeySession(m_kKeySessionId));

    // Session should still exist, so any function call should be executed:
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_mediaKeySessionMock, removeKeySession()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_mediaKeys->removeKeySession(m_kKeySessionId));
}
