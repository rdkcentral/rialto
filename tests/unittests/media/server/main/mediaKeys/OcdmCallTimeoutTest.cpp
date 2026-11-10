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

#include "HeartbeatHandlerMock.h"
#include "MainThread.h"
#include "MediaKeySessionFactoryMock.h"
#include "MediaKeySessionMock.h"
#include "MediaKeysServerInternal.h"
#include "OcdmSystemFactoryMock.h"
#include "OcdmSystemMock.h"
#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <future>
#include <memory>
#include <string>
#include <thread>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;
using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{
const std::string kKeySystem{"com.widevine.alpha"};
constexpr std::chrono::milliseconds kTimeout{100};
constexpr std::chrono::milliseconds kFailFastLimit{50};
constexpr uint64_t kDrmTime{1234};

class BlockingDestructionSessionMock : public MediaKeySessionMock
{
public:
    explicit BlockingDestructionSessionMock(std::shared_future<void> released) : m_released{std::move(released)} {}
    ~BlockingDestructionSessionMock() override { m_released.wait(); }

private:
    std::shared_future<void> m_released;
};
} // namespace

class RialtoServerMediaKeysOcdmCallTimeoutTest : public ::testing::Test
{
protected:
    std::shared_ptr<DedicatedMainThreadFactory> m_mainThreadFactory{std::make_shared<DedicatedMainThreadFactory>()};
    std::shared_ptr<StrictMock<OcdmSystemFactoryMock>> m_ocdmSystemFactoryMock{
        std::make_shared<StrictMock<OcdmSystemFactoryMock>>()};
    std::shared_ptr<StrictMock<OcdmSystemMock>> m_ocdmSystemMock{std::make_shared<StrictMock<OcdmSystemMock>>()};
    std::shared_ptr<StrictMock<MediaKeySessionFactoryMock>> m_mediaKeySessionFactoryMock{
        std::make_shared<StrictMock<MediaKeySessionFactoryMock>>()};
    std::unique_ptr<MediaKeysServerInternal> m_sut;
    std::promise<void> m_release;
    std::shared_future<void> m_released{m_release.get_future().share()};

    ~RialtoServerMediaKeysOcdmCallTimeoutTest() override { releaseOcdm(); }

    void createMediaKeys()
    {
        EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kKeySystem)).WillOnce(Return(m_ocdmSystemMock));
        m_sut = std::make_unique<MediaKeysServerInternal>(kKeySystem, m_mainThreadFactory, m_ocdmSystemFactoryMock,
                                                          m_mediaKeySessionFactoryMock, kTimeout);
    }

    int32_t createKeySession(std::unique_ptr<IMediaKeySession> session)
    {
        EXPECT_CALL(*m_mediaKeySessionFactoryMock, createMediaKeySession(kKeySystem, _, _, KeySessionType::TEMPORARY, _))
            .WillOnce(Return(ByMove(std::move(session))));
        int32_t keySessionId{-1};
        EXPECT_EQ(MediaKeyErrorStatus::OK,
                  m_sut->createKeySession(KeySessionType::TEMPORARY, std::weak_ptr<IMediaKeysClient>{}, keySessionId));
        return keySessionId;
    }

    void releaseOcdm()
    {
        try
        {
            m_release.set_value();
        }
        catch (const std::future_error &)
        {
        }
    }

    void waitUntilOcdmResponsive()
    {
        for (int i = 0; i < 200 && m_sut->isOcdmCallBlocked(); ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
        EXPECT_FALSE(m_sut->isOcdmCallBlocked());
    }

    template <typename F> static std::chrono::milliseconds measure(F &&function)
    {
        const auto kStart = std::chrono::steady_clock::now();
        function();
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - kStart);
    }
};

TEST_F(RialtoServerMediaKeysOcdmCallTimeoutTest, ShouldReadTimeoutFromEnvironment)
{
    unsetenv("RIALTO_OCDM_CALL_TIMEOUT_MS");
    EXPECT_EQ(std::chrono::milliseconds{0}, MediaKeysServerInternal::getOcdmCallTimeoutFromEnv());
    setenv("RIALTO_OCDM_CALL_TIMEOUT_MS", "2500", 1);
    EXPECT_EQ(std::chrono::milliseconds{2500}, MediaKeysServerInternal::getOcdmCallTimeoutFromEnv());
    for (const char *invalid : {"", "abc", "-1", "10ms", "99999999999999999999"})
    {
        setenv("RIALTO_OCDM_CALL_TIMEOUT_MS", invalid, 1);
        EXPECT_EQ(std::chrono::milliseconds{0}, MediaKeysServerInternal::getOcdmCallTimeoutFromEnv()) << invalid;
    }
    unsetenv("RIALTO_OCDM_CALL_TIMEOUT_MS");
}

TEST_F(RialtoServerMediaKeysOcdmCallTimeoutTest, ShouldFailConstructionWhenCreateOcdmSystemBlocks)
{
    auto released = m_released;
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kKeySystem))
        .WillOnce(Invoke(
            [released](const std::string &)
            {
                released.wait();
                return nullptr;
            }));

    const auto kDuration = measure(
        [this]()
        {
            EXPECT_THROW(m_sut = std::make_unique<MediaKeysServerInternal>(kKeySystem, m_mainThreadFactory,
                                                                           m_ocdmSystemFactoryMock,
                                                                           m_mediaKeySessionFactoryMock, kTimeout),
                         std::runtime_error);
        });
    EXPECT_GE(kDuration, kTimeout);
    EXPECT_LT(kDuration, kTimeout * 10);
}

TEST_F(RialtoServerMediaKeysOcdmCallTimeoutTest, ShouldFailBlockedCallFailFastAndRecover)
{
    createMediaKeys();
    auto released = m_released;
    EXPECT_CALL(*m_ocdmSystemMock, getDrmTime(_))
        .WillOnce(Invoke(
            [released](uint64_t *drmTime)
            {
                released.wait();
                *drmTime = 0;
                return MediaKeyErrorStatus::OK;
            }))
        .WillOnce(Invoke(
            [](uint64_t *drmTime)
            {
                *drmTime = kDrmTime;
                return MediaKeyErrorStatus::OK;
            }));

    uint64_t drmTime{0};
    auto duration = measure([&]() { EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_sut->getDrmTime(drmTime)); });
    EXPECT_GE(duration, kTimeout);
    EXPECT_TRUE(m_sut->isOcdmCallBlocked());

    // Calls fail immediately instead of queueing behind the blocked call
    duration = measure([&]() { EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_sut->getDrmTime(drmTime)); });
    EXPECT_LT(duration, kFailFastLimit);
    duration = measure([&]() { EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_sut->closeKeySession(1)); });
    EXPECT_LT(duration, kFailFastLimit);

    // Heartbeat is still acknowledged
    duration = measure([&]() { m_sut->ping(std::make_unique<StrictMock<HeartbeatHandlerMock>>()); });
    EXPECT_LT(duration, kFailFastLimit);

    releaseOcdm();
    waitUntilOcdmResponsive();

    EXPECT_EQ(MediaKeyErrorStatus::OK, m_sut->getDrmTime(drmTime));
    EXPECT_EQ(kDrmTime, drmTime);
    m_sut.reset();
}

TEST_F(RialtoServerMediaKeysOcdmCallTimeoutTest, ShouldFailBlockedDecryptAndKeepBufferAlive)
{
    if (!gst_is_initialized())
    {
        gst_init(nullptr, nullptr);
    }
    createMediaKeys();
    auto session = std::make_unique<StrictMock<MediaKeySessionMock>>();
    auto released = m_released;
    EXPECT_CALL(*session, decrypt(_, _))
        .WillOnce(Invoke(
            [released](GstBuffer *, GstCaps *)
            {
                released.wait();
                return MediaKeyErrorStatus::OK;
            }));
    const int32_t kKeySessionId{createKeySession(std::move(session))};
    GstBuffer *buffer{gst_buffer_new()};
    GstCaps *caps{gst_caps_new_empty()};

    auto duration = measure([&]() { EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_sut->decrypt(kKeySessionId, buffer, caps)); });
    EXPECT_GE(duration, kTimeout);
    EXPECT_TRUE(m_sut->isOcdmCallBlocked());
    // The blocked decrypt still holds its own references
    EXPECT_EQ(2, GST_MINI_OBJECT_REFCOUNT_VALUE(buffer));
    EXPECT_EQ(2, GST_MINI_OBJECT_REFCOUNT_VALUE(caps));

    releaseOcdm();
    waitUntilOcdmResponsive();
    for (int i = 0; i < 200 && GST_MINI_OBJECT_REFCOUNT_VALUE(buffer) > 1; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    EXPECT_EQ(1, GST_MINI_OBJECT_REFCOUNT_VALUE(buffer));
    EXPECT_EQ(1, GST_MINI_OBJECT_REFCOUNT_VALUE(caps));
    gst_buffer_unref(buffer);
    gst_caps_unref(caps);
    m_sut.reset();
}

TEST_F(RialtoServerMediaKeysOcdmCallTimeoutTest, ShouldNotHangDestructionWhenKeySessionDestructionBlocks)
{
    createMediaKeys();
    createKeySession(std::make_unique<BlockingDestructionSessionMock>(m_released));

    const auto kDuration = measure([&]() { m_sut.reset(); });
    EXPECT_GE(kDuration, kTimeout);
    EXPECT_LT(kDuration, kTimeout * 10);
}

TEST_F(RialtoServerMediaKeysOcdmCallTimeoutTest, ShouldSkipCallThatCouldNotStartInTime)
{
    createMediaKeys();
    auto released = m_released;
    m_mainThreadFactory->getMainThread()->enqueueTask(0, [released]() { released.wait(); });

    // getDrmTime must never reach OCDM: the StrictMock fails the test if it does
    uint64_t drmTime{0};
    EXPECT_EQ(MediaKeyErrorStatus::FAIL, m_sut->getDrmTime(drmTime));
    EXPECT_TRUE(m_sut->isOcdmCallBlocked());

    releaseOcdm();
    waitUntilOcdmResponsive();
    m_sut.reset();
}

TEST_F(RialtoServerMediaKeysOcdmCallTimeoutTest, ShouldNotBoundCallsWhenTimeoutIsDisabled)
{
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kKeySystem)).WillOnce(Return(m_ocdmSystemMock));
    m_sut = std::make_unique<MediaKeysServerInternal>(kKeySystem, m_mainThreadFactory, m_ocdmSystemFactoryMock,
                                                      m_mediaKeySessionFactoryMock);
    EXPECT_CALL(*m_ocdmSystemMock, getDrmTime(_))
        .WillOnce(Invoke(
            [](uint64_t *drmTime)
            {
                std::this_thread::sleep_for(kTimeout * 2);
                *drmTime = kDrmTime;
                return MediaKeyErrorStatus::OK;
            }));

    uint64_t drmTime{0};
    EXPECT_EQ(MediaKeyErrorStatus::OK, m_sut->getDrmTime(drmTime));
    EXPECT_EQ(kDrmTime, drmTime);
    EXPECT_FALSE(m_sut->isOcdmCallBlocked());
    m_sut.reset();
}
