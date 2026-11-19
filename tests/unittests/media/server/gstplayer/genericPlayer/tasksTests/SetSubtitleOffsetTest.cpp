/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "GenericTasksTestsBase.h"
#include "GstRialtoTextTrackSinkPrivate.h"

#include <limits>

using testing::Return;
using testing::StrictMock;

class TextTrackSessionMock : public firebolt::rialto::server::ITextTrackSession
{
public:
    MOCK_METHOD(bool, resetSession, (bool), (override));
    MOCK_METHOD(bool, pause, (), (override));
    MOCK_METHOD(bool, play, (), (override));
    MOCK_METHOD(bool, mute, (bool), (override));
    MOCK_METHOD(bool, setPosition, (uint64_t), (override));
    MOCK_METHOD(bool, sendData, (const std::string &, int64_t), (override));
    MOCK_METHOD(bool, setSessionWebVTTSelection, (), (override));
    MOCK_METHOD(bool, setSessionTTMLSelection, (), (override));
    MOCK_METHOD(bool, setSessionCCSelection, (const std::string &), (override));
    MOCK_METHOD(bool, associateVideoDecoder, (gpointer), (override));
    MOCK_METHOD(bool, isClosedCaptions, (), (const, override));
};

class SetSubtitleOffsetTest : public GenericTasksTestsBase
{
protected:
    SetSubtitleOffsetTest() { setContextPlaybackRate(); }
};

TEST_F(SetSubtitleOffsetTest, ShouldNotSetSubtitleOffsetWhenSubtitleSinkIsNull)
{
    triggerSetSubtitleOffset();
    // Should not set any offset since subtitle sink is not set
}

TEST_F(SetSubtitleOffsetTest, ShouldSetSubtitleOffsetWithSubtitleSink)
{
    setContextSubtitleSink();
    shouldSetSubtitleOffset();
    triggerSetSubtitleOffset();
}

TEST_F(SetSubtitleOffsetTest, ShouldSetNegativeSubtitleOffsetWithSubtitleSink)
{
    constexpr int64_t kNegativeOffset{-1000};
    setContextSubtitleSink();
    shouldSetSubtitleOffset(kNegativeOffset);
    triggerSetSubtitleOffset(kNegativeOffset);
}

TEST(SubtitlePositionWithOffsetTest, HandlesNegativeOffsetsAndClampsBelowZero)
{
    EXPECT_EQ(1000U, firebolt::rialto::server::calculateSubtitlePosition(2000U, -1000));
    EXPECT_EQ(0U, firebolt::rialto::server::calculateSubtitlePosition(1000U, -1000));
    EXPECT_EQ(0U, firebolt::rialto::server::calculateSubtitlePosition(500U, -1000));
}

TEST(SubtitlePositionWithOffsetTest, AppliesNegativeOffsetBeforeSettingSessionPosition)
{
    constexpr uint64_t kPosition{2 * GST_SECOND};
    constexpr int64_t kOffset{-static_cast<int64_t>(GST_SECOND)};
    StrictMock<TextTrackSessionMock> session;
    EXPECT_CALL(session, setPosition(1000)).WillOnce(Return(true));

    EXPECT_EQ(GST_SECOND, firebolt::rialto::server::setSubtitlePosition(session, kPosition, kOffset));
}

TEST(SubtitlePositionWithOffsetTest, HandlesOffsetLimitsWithoutOverflow)
{
    constexpr uint64_t kMaxPosition{std::numeric_limits<uint64_t>::max()};
    constexpr int64_t kMinOffset{std::numeric_limits<int64_t>::min()};
    constexpr int64_t kMaxOffset{std::numeric_limits<int64_t>::max()};

    EXPECT_EQ(std::numeric_limits<uint64_t>::max(),
              firebolt::rialto::server::calculateSubtitlePosition(kMaxPosition - 1, 2));
    EXPECT_EQ(static_cast<uint64_t>(std::numeric_limits<int64_t>::max()),
              firebolt::rialto::server::calculateSubtitlePosition(kMaxPosition, kMinOffset));
    EXPECT_EQ(kMaxPosition, firebolt::rialto::server::calculateSubtitlePosition(kMaxPosition, kMaxOffset));
}
