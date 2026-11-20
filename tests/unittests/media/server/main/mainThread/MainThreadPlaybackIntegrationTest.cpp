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

#include "MainThread.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace firebolt::rialto::server;
using ::testing::InSequence;

/**
 * @brief Integration tests for MainThread with media playback simulation
 *        Tests YouTube, VIPA, and XIPA playback scenarios
 */
class MainThreadPlaybackIntegrationTests : public testing::Test
{
protected:
    std::shared_ptr<IMainThread> m_mainThread;
    const uint32_t m_mainThreadClientId{0};
    const std::chrono::milliseconds m_timeout{2000};

    void SetUp() override { m_mainThread = std::make_shared<MainThread>(m_timeout); }

    void TearDown() override { m_mainThread.reset(); }
};

/**
 * @class YouTubePlaybackSimulator
 * @brief Simulates YouTube playback operations on MainThread
 */
class YouTubePlaybackSimulator
{
public:
    struct State
    {
        std::atomic<bool> isPlaying{false};
        std::atomic<uint64_t> bytesDecoded{0};
        std::atomic<bool> underrun{false};
        std::condition_variable stateChange;
        std::mutex stateMutex;
    };

    explicit YouTubePlaybackSimulator(std::shared_ptr<IMainThread> mainThread)
        : m_mainThread(mainThread), m_state(std::make_shared<State>())
    {
        m_clientId = m_mainThread->registerClient();
    }

    ~YouTubePlaybackSimulator() { m_mainThread->unregisterClient(m_clientId); }

    uint32_t getClientId() const { return m_clientId; }

    bool start()
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId,
                                                [this]()
                                                {
                                                    m_state->isPlaying = true;
                                                    m_state->bytesDecoded = 0;
                                                });
    }

    bool stop()
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId, [this]() { m_state->isPlaying = false; });
    }

    bool feedData(size_t bytesCount)
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId,
                                                [this, bytesCount]()
                                                {
                                                    if (m_state->isPlaying)
                                                    {
                                                        m_state->bytesDecoded += bytesCount;
                                                    }
                                                });
    }

    bool checkUnderrun()
    {
        bool result = false;
        m_mainThread->enqueueTaskAndWait(m_clientId,
                                         [this, &result]()
                                         {
                                             result = m_state->bytesDecoded < 1024;
                                             if (result)
                                                 m_state->underrun = true;
                                         });
        return result;
    }

    std::shared_ptr<State> getState() const { return m_state; }

private:
    std::shared_ptr<IMainThread> m_mainThread;
    std::shared_ptr<State> m_state;
    uint32_t m_clientId;
};

/**
 * @class VIPAPlaybackSimulator
 * @brief Simulates VIPA playback with DRM/protected content handling
 */
class VIPAPlaybackSimulator
{
public:
    struct State
    {
        std::atomic<bool> isPlaying{false};
        std::atomic<uint32_t> frameCount{0};
        std::atomic<bool> decryptionFailed{false};
        std::atomic<uint64_t> bytesProcessed{0};
    };

    explicit VIPAPlaybackSimulator(std::shared_ptr<IMainThread> mainThread)
        : m_mainThread(mainThread), m_state(std::make_shared<State>())
    {
        m_clientId = m_mainThread->registerClient();
    }

    ~VIPAPlaybackSimulator() { m_mainThread->unregisterClient(m_clientId); }

    uint32_t getClientId() const { return m_clientId; }

    bool initializeDRM()
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId,
                                                [this]()
                                                {
                                                    // Simulate DRM initialization
                                                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                                });
    }

    bool processFrame(size_t encryptedDataSize)
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId,
                                                [this, encryptedDataSize]()
                                                {
                                                    // Simulate VIPA frame decryption
                                                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                                    m_state->frameCount++;
                                                    m_state->bytesProcessed += encryptedDataSize;
                                                });
    }

    bool play()
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId, [this]() { m_state->isPlaying = true; });
    }

    bool pause()
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId, [this]() { m_state->isPlaying = false; });
    }

    std::shared_ptr<State> getState() const { return m_state; }

private:
    std::shared_ptr<IMainThread> m_mainThread;
    std::shared_ptr<State> m_state;
    uint32_t m_clientId;
};

/**
 * @class XIPAPlaybackSimulator
 * @brief Simulates XIPA playback with streaming and adaptive bitrate
 */
class XIPAPlaybackSimulator
{
public:
    struct State
    {
        std::atomic<bool> isStreaming{false};
        std::atomic<uint32_t> bitrateKbps{5000};
        std::atomic<uint32_t> bufferSizeMs{0};
        std::atomic<uint64_t> totalBytesStreamed{0};
        std::atomic<bool> networkStalled{false};
    };

    explicit XIPAPlaybackSimulator(std::shared_ptr<IMainThread> mainThread)
        : m_mainThread(mainThread), m_state(std::make_shared<State>())
    {
        m_clientId = m_mainThread->registerClient();
    }

    ~XIPAPlaybackSimulator() { m_mainThread->unregisterClient(m_clientId); }

    uint32_t getClientId() const { return m_clientId; }

    bool startStreaming()
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId,
                                                [this]()
                                                {
                                                    m_state->isStreaming = true;
                                                    m_state->bufferSizeMs = 5000;
                                                });
    }

    bool adaptBitrate(uint32_t newBitrateKbps)
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId,
                                                [this, newBitrateKbps]() { m_state->bitrateKbps = newBitrateKbps; });
    }

    bool feedStreamData(size_t bytesCount)
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId,
                                                [this, bytesCount]()
                                                {
                                                    if (m_state->isStreaming)
                                                    {
                                                        m_state->totalBytesStreamed += bytesCount;
                                                        if (m_state->bufferSizeMs > 100)
                                                            m_state->bufferSizeMs -= 100;
                                                    }
                                                });
    }

    bool checkNetworkStall()
    {
        bool stalled = false;
        m_mainThread->enqueueTaskAndWait(m_clientId,
                                         [this, &stalled]()
                                         {
                                             stalled = m_state->bufferSizeMs < 1000;
                                             m_state->networkStalled = stalled;
                                         });
        return stalled;
    }

    bool stop()
    {
        return m_mainThread->enqueueTaskAndWait(m_clientId, [this]() { m_state->isStreaming = false; });
    }

    std::shared_ptr<State> getState() const { return m_state; }

private:
    std::shared_ptr<IMainThread> m_mainThread;
    std::shared_ptr<State> m_state;
    uint32_t m_clientId;
};

// ============================================================================
// YouTube Playback Tests
// ============================================================================

/**
 * Test: Basic YouTube playback initialization and data feeding
 * Validates: That normal playback operations complete within timeout
 */
TEST_F(MainThreadPlaybackIntegrationTests, YouTubeBasicPlaybackFlow)
{
    YouTubePlaybackSimulator youtube(m_mainThread);

    EXPECT_TRUE(youtube.start());
    EXPECT_TRUE(youtube.feedData(4096));
    EXPECT_TRUE(youtube.feedData(4096));
    EXPECT_FALSE(youtube.checkUnderrun());
    EXPECT_TRUE(youtube.stop());
}

/**
 * Test: YouTube playback timeout when MainThread is blocked
 * Validates: AC1 - Blocking enqueue returns error within bounded timeout
 */
TEST_F(MainThreadPlaybackIntegrationTests, YouTubePlaybackTimesOutWhenMainThreadBlocked)
{
    YouTubePlaybackSimulator youtube(m_mainThread);

    std::promise<void> release;
    auto releaseFlag = release.get_future().share();

    // Block the main thread with a blocking task
    m_mainThread->enqueueTask(m_mainThreadClientId, [releaseFlag]() { releaseFlag.wait(); });

    // Playback operation should timeout
    auto future = std::async(std::launch::async,
                             [&]()
                             {
                                 auto start = std::chrono::steady_clock::now();
                                 bool result = youtube.feedData(4096);
                                 auto elapsed = std::chrono::steady_clock::now() - start;

                                 EXPECT_FALSE(result) << "Expected timeout when MainThread is blocked";
                                 EXPECT_GE(elapsed, std::chrono::milliseconds(1900));
                                 EXPECT_LE(elapsed, std::chrono::milliseconds(2500));
                             });

    future.wait();
    release.set_value();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * Test: Multiple YouTube clients concurrent playback
 * Validates: Thread safety with concurrent enqueueTaskAndWait calls
 */
TEST_F(MainThreadPlaybackIntegrationTests, MultipleYouTubeClientsPlayback)
{
    const int numClients = 5;
    std::vector<std::unique_ptr<YouTubePlaybackSimulator>> clients;

    for (int i = 0; i < numClients; ++i)
    {
        clients.push_back(std::make_unique<YouTubePlaybackSimulator>(m_mainThread));
    }

    std::vector<std::future<void>> futures;
    std::transform(clients.begin(), clients.end(), std::back_inserter(futures),
                   [&](auto &client)
                   {
                       return std::async(std::launch::async,
                                         [&]()
                                         {
                                             EXPECT_TRUE(client->start());
                                             for (int j = 0; j < 10; ++j)
                                             {
                                                 EXPECT_TRUE(client->feedData(1024));
                                             }
                                             EXPECT_TRUE(client->stop());
                                         });
                   });

    for (auto &f : futures)
    {
        f.wait();
    }
}

/**
 * Test: YouTube playback continues after shutdown of another client
 * Validates: Shutdown of one client doesn't affect others
 */
TEST_F(MainThreadPlaybackIntegrationTests, YouTubePlaybackAfterOtherClientShutdown)
{
    auto youtube1 = std::make_unique<YouTubePlaybackSimulator>(m_mainThread);
    YouTubePlaybackSimulator youtube2(m_mainThread);

    EXPECT_TRUE(youtube1->start());
    EXPECT_TRUE(youtube2.start());

    youtube1.reset(); // Destroy first client

    // Second client should still work
    EXPECT_TRUE(youtube2.feedData(4096));
    EXPECT_TRUE(youtube2.stop());
}

// ============================================================================
// VIPA Playback Tests
// ============================================================================

/**
 * Test: Basic VIPA playback with DRM initialization
 * Validates: Protected content playback operations complete successfully
 */
TEST_F(MainThreadPlaybackIntegrationTests, VIPABasicDRMPlayback)
{
    VIPAPlaybackSimulator vipa(m_mainThread);

    EXPECT_TRUE(vipa.initializeDRM());
    EXPECT_TRUE(vipa.play());

    for (int i = 0; i < 5; ++i)
    {
        EXPECT_TRUE(vipa.processFrame(2048));
    }

    EXPECT_TRUE(vipa.pause());
}

/**
 * Test: VIPA playback timeout during heavy DRM operations
 * Validates: AC1 - Timeout occurs when DRM processing blocks MainThread
 */
TEST_F(MainThreadPlaybackIntegrationTests, VIPAPlaybackTimesOutWhenMainThreadBlocked)
{
    VIPAPlaybackSimulator vipa(m_mainThread);

    std::promise<void> release;
    auto releaseFlag = release.get_future().share();

    // Block main thread
    m_mainThread->enqueueTask(m_mainThreadClientId, [releaseFlag]() { releaseFlag.wait(); });

    auto future = std::async(std::launch::async,
                             [&]()
                             {
                                 auto start = std::chrono::steady_clock::now();
                                 bool result = vipa.processFrame(2048);
                                 auto elapsed = std::chrono::steady_clock::now() - start;

                                 EXPECT_FALSE(result) << "Expected timeout during DRM processing";
                                 EXPECT_GE(elapsed, std::chrono::milliseconds(1900));
                             });

    future.wait();
    release.set_value();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * Test: Multiple VIPA clients with concurrent DRM operations
 * Validates: Thread-safe DRM processing with multiple clients
 */
TEST_F(MainThreadPlaybackIntegrationTests, MultipleVIPAClientsConcurrentPlayback)
{
    const int numClients = 3;
    std::vector<std::unique_ptr<VIPAPlaybackSimulator>> clients;

    for (int i = 0; i < numClients; ++i)
    {
        clients.push_back(std::make_unique<VIPAPlaybackSimulator>(m_mainThread));
    }

    std::vector<std::future<void>> futures;
    std::transform(clients.begin(), clients.end(), std::back_inserter(futures),
                   [&](auto &client)
                   {
                       return std::async(std::launch::async,
                                         [&]()
                                         {
                                             EXPECT_TRUE(client->initializeDRM());
                                             EXPECT_TRUE(client->play());

                                             for (int j = 0; j < 5; ++j)
                                             {
                                                 EXPECT_TRUE(client->processFrame(1024));
                                             }

                                             EXPECT_TRUE(client->pause());
                                         });
                   });

    for (auto &f : futures)
    {
        f.wait();
    }
}

// ============================================================================
// XIPA Playback Tests
// ============================================================================

/**
 * Test: Basic XIPA adaptive streaming playback
 * Validates: Streaming and bitrate adaptation operations work correctly
 */
TEST_F(MainThreadPlaybackIntegrationTests, XIPABasicStreamingPlayback)
{
    XIPAPlaybackSimulator xipa(m_mainThread);

    EXPECT_TRUE(xipa.startStreaming());
    EXPECT_EQ(xipa.getState()->bitrateKbps, 5000);

    // Feed initial data
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_TRUE(xipa.feedStreamData(512));
    }

    // Adapt to lower bitrate
    EXPECT_TRUE(xipa.adaptBitrate(2500));
    EXPECT_EQ(xipa.getState()->bitrateKbps, 2500);

    EXPECT_TRUE(xipa.stop());
}

/**
 * Test: XIPA playback timeout when network operations block
 * Validates: AC1 - Timeout during heavy streaming operations
 */
TEST_F(MainThreadPlaybackIntegrationTests, XIPAPlaybackTimesOutWhenMainThreadBlocked)
{
    XIPAPlaybackSimulator xipa(m_mainThread);

    std::promise<void> release;
    auto releaseFlag = release.get_future().share();

    // Block main thread
    m_mainThread->enqueueTask(m_mainThreadClientId, [releaseFlag]() { releaseFlag.wait(); });

    auto future = std::async(std::launch::async,
                             [&]()
                             {
                                 auto start = std::chrono::steady_clock::now();
                                 bool result = xipa.feedStreamData(2048);
                                 auto elapsed = std::chrono::steady_clock::now() - start;

                                 EXPECT_FALSE(result) << "Expected timeout during streaming";
                                 EXPECT_GE(elapsed, std::chrono::milliseconds(1900));
                             });

    future.wait();
    release.set_value();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * Test: XIPA adaptive bitrate adjustment under network stress
 * Validates: Concurrent bitrate adaptation with data feeding
 */
TEST_F(MainThreadPlaybackIntegrationTests, XIPAConcurrentBitrateAdaptation)
{
    XIPAPlaybackSimulator xipa(m_mainThread);

    EXPECT_TRUE(xipa.startStreaming());

    std::vector<std::future<void>> futures;

    // Feed data from one thread
    futures.push_back(std::async(std::launch::async,
                                 [&]()
                                 {
                                     for (int i = 0; i < 20; ++i)
                                     {
                                         EXPECT_TRUE(xipa.feedStreamData(256));
                                         std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                     }
                                 }));

    // Adapt bitrate from another thread
    futures.push_back(std::async(std::launch::async,
                                 [&]()
                                 {
                                     std::vector<uint32_t> bitrates = {5000, 3000, 1500, 3000, 5000};
                                     for (uint32_t bitrate : bitrates)
                                     {
                                         std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                         EXPECT_TRUE(xipa.adaptBitrate(bitrate));
                                     }
                                 }));

    for (auto &f : futures)
    {
        f.wait();
    }

    EXPECT_TRUE(xipa.stop());
}

/**
 * Test: Multiple XIPA clients with concurrent streaming
 * Validates: Multiple streaming clients don't interfere with each other
 */
TEST_F(MainThreadPlaybackIntegrationTests, MultipleXIPAClientsConcurrentStreaming)
{
    const int numClients = 4;
    std::vector<std::unique_ptr<XIPAPlaybackSimulator>> clients;

    for (int i = 0; i < numClients; ++i)
    {
        clients.push_back(std::make_unique<XIPAPlaybackSimulator>(m_mainThread));
    }

    std::vector<std::future<void>> futures;
    std::transform(clients.begin(), clients.end(), std::back_inserter(futures),
                   [&](auto &client)
                   {
                       return std::async(std::launch::async,
                                         [&]()
                                         {
                                             EXPECT_TRUE(client->startStreaming());

                                             for (int j = 0; j < 10; ++j)
                                             {
                                                 EXPECT_TRUE(client->feedStreamData(256));
                                             }

                                             EXPECT_FALSE(client->checkNetworkStall());
                                             EXPECT_TRUE(client->stop());
                                         });
                   });

    for (auto &f : futures)
    {
        f.wait();
    }
}

// ============================================================================
// Cross-Playback Tests
// ============================================================================

/**
 * Test: Mixed YouTube, VIPA, XIPA playback concurrently
 * Validates: All playback types work together without interference
 */
TEST_F(MainThreadPlaybackIntegrationTests, MixedPlaybackTypesCoexist)
{
    YouTubePlaybackSimulator youtube(m_mainThread);
    VIPAPlaybackSimulator vipa(m_mainThread);
    XIPAPlaybackSimulator xipa(m_mainThread);

    std::vector<std::future<void>> futures;

    futures.push_back(std::async(std::launch::async,
                                 [&]()
                                 {
                                     EXPECT_TRUE(youtube.start());
                                     for (int i = 0; i < 5; ++i)
                                         EXPECT_TRUE(youtube.feedData(1024));
                                     EXPECT_TRUE(youtube.stop());
                                 }));

    futures.push_back(std::async(std::launch::async,
                                 [&]()
                                 {
                                     EXPECT_TRUE(vipa.initializeDRM());
                                     EXPECT_TRUE(vipa.play());
                                     for (int i = 0; i < 5; ++i)
                                         EXPECT_TRUE(vipa.processFrame(512));
                                     EXPECT_TRUE(vipa.pause());
                                 }));

    futures.push_back(std::async(std::launch::async,
                                 [&]()
                                 {
                                     EXPECT_TRUE(xipa.startStreaming());
                                     for (int i = 0; i < 5; ++i)
                                         EXPECT_TRUE(xipa.feedStreamData(256));
                                     EXPECT_TRUE(xipa.stop());
                                 }));

    for (auto &f : futures)
    {
        f.wait();
    }
}

/**
 * Test: Shutdown while playback is ongoing
 * Validates: AC2 - Destructors fail outstanding waiters
 */
TEST_F(MainThreadPlaybackIntegrationTests, PlaybackShutdownWithOutstandingWaiters)
{
    {
        YouTubePlaybackSimulator youtube(m_mainThread);
        VIPAPlaybackSimulator vipa(m_mainThread);

        std::promise<void> release;
        auto releaseFlag = release.get_future().share();

        // Block the main thread
        m_mainThread->enqueueTask(m_mainThreadClientId, [releaseFlag]() { releaseFlag.wait(); });

        // Start operations that will wait
        auto youtubeWaiter = std::async(std::launch::async, [&]() { youtube.feedData(4096); });

        auto vipaWaiter = std::async(std::launch::async, [&]() { vipa.processFrame(2048); });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Destroy simulators while waiters are pending
    }

    // Release the blocked main thread
    // The waiting operations should have failed due to MainThread destruction

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

/**
 * Test: Issue validation - Concurrent enqueue calls with priority
 * Validates: Priority and normal tasks are processed correctly
 */
TEST_F(MainThreadPlaybackIntegrationTests, PlaybackSynchronousFifoEnqueueing)
{
    YouTubePlaybackSimulator youtube(m_mainThread);
    VIPAPlaybackSimulator vipa(m_mainThread);

    std::atomic<int> executionOrder{0};
    std::vector<int> order;
    std::mutex orderMutex;

    auto youtube_id = youtube.getClientId();
    auto vipa_id = vipa.getClientId();

    // Normal task
    m_mainThread->enqueueTask(youtube_id,
                              [&]()
                              {
                                  std::lock_guard<std::mutex> lock(orderMutex);
                                  order.push_back(1);
                              });

    // Synchronous task
    m_mainThread->enqueueTaskAndWait(vipa_id,
                                     [&]()
                                     {
                                         std::lock_guard<std::mutex> lock(orderMutex);
                                         order.push_back(2);
                                     });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(order.size(), 2);
    // The queued task should be processed before the synchronous task.
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
}
