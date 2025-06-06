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

#include "FlushWatcher.h"
#include <gtest/gtest.h>

namespace firebolt::rialto::server
{
TEST(FlushWatcherTests, ShouldReturnCorrectValue)
{
    FlushWatcher watcher;

    // Not flushing after startup
    EXPECT_FALSE(watcher.isFlushOngoing());

    // Flushing when flushing set for at least one source
    watcher.setFlushing(MediaSourceType::AUDIO, false);
    EXPECT_TRUE(watcher.isFlushOngoing());
    EXPECT_FALSE(watcher.isAsyncFlushOngoing());

    watcher.setFlushing(MediaSourceType::VIDEO, true);
    EXPECT_TRUE(watcher.isFlushOngoing());
    EXPECT_TRUE(watcher.isAsyncFlushOngoing());

    // Not flushing when all flush flags cleared
    watcher.setFlushed(MediaSourceType::AUDIO);
    EXPECT_TRUE(watcher.isFlushOngoing());
    EXPECT_TRUE(watcher.isAsyncFlushOngoing());

    watcher.setFlushed(MediaSourceType::VIDEO);
    EXPECT_FALSE(watcher.isFlushOngoing());
    EXPECT_FALSE(watcher.isAsyncFlushOngoing());
}
} // namespace firebolt::rialto::server
