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

#include <atomic>
#include <condition_variable>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mutex>

#include "IEventThread.h"

namespace
{
const std::string kThreadName("test");
constexpr int kNumEvents{10};
constexpr std::chrono::milliseconds kEnoughTimeForTestToComplete{200};

class TestFunction
{
public:
    TestFunction(std::mutex &mtx, std::condition_variable &cv, bool &flag) : m_mutex{mtx}, m_cv{cv}, m_callFlag{flag} {}
    ~TestFunction() = default;

    void test()
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        m_callFlag = true;
        m_cv.notify_one();
    }

private:
    std::mutex &m_mutex;
    std::condition_variable &m_cv;
    bool &m_callFlag;
};
void testSetBool(bool *x)
{
    usleep(1000);
    *x = true;
}
} // namespace

class EventThreadTests : public testing::Test
{
public:
    EventThreadTests() = default;

protected:
    std::unique_ptr<firebolt::rialto::common::IEventThread> m_sut{
        firebolt::rialto::common::IEventThreadFactory::createFactory()->createEventThread(kThreadName)};
};

TEST_F(EventThreadTests, EventThreadShouldCallInlineFunction)
{
    std::mutex mtx;
    { // This scope is required to suppress a false warning from cppcheck (about the mutex above)
        std::unique_lock<std::mutex> lock{mtx};
        std::condition_variable cv;
        bool callFlag{false};
        TestFunction tf(mtx, cv, callFlag);

        m_sut->add(
            [&]()
            {
                std::unique_lock<std::mutex> lock;
                callFlag = true;
                cv.notify_one();
            });
        cv.wait_for(lock, std::chrono::milliseconds(kEnoughTimeForTestToComplete));
        EXPECT_TRUE(callFlag);
    }
}

TEST_F(EventThreadTests, EventThreadShouldCallClassMethod)
{
    std::mutex mtx;
    { // This scope is required to suppress a false warning from cppcheck (about the mutex above)
        std::unique_lock<std::mutex> lock{mtx};
        std::condition_variable cv;
        bool callFlag{false};
        TestFunction tf(mtx, cv, callFlag);

        m_sut->add(&TestFunction::test, &tf);
        cv.wait_for(lock, std::chrono::milliseconds(kEnoughTimeForTestToComplete));
        EXPECT_TRUE(callFlag);
    }
}

TEST_F(EventThreadTests, FlushShouldWaitForAllEventsToComplete)
{
    bool callFlags[kNumEvents];
    for (int i = 0; i < kNumEvents; ++i)
    {
        callFlags[i] = false;
        m_sut->add(testSetBool, &callFlags[i]);
    }
    for (int i = 0; i < kNumEvents; ++i)
    {
        EXPECT_FALSE(callFlags[i]);
    }
    m_sut->flush();
    for (int i = 0; i < kNumEvents; ++i)
    {
        EXPECT_TRUE(callFlags[i]);
    }
}
