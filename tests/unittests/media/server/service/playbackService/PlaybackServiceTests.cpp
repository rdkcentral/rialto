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

#include "PlaybackServiceTestsFixture.h"

TEST_F(PlaybackServiceTests, shouldFailToGetSharedMemoryInInactiveState)
{
    createPlaybackServiceShouldSuccess();
    getSharedMemoryShouldFail();
    getShmBufferShouldFail();
}

TEST_F(PlaybackServiceTests, shouldGetSharedMemory)
{
    createPlaybackServiceShouldSuccess();
    triggerSetMaxPlaybacks();
    triggerSetMaxWebAudioPlayers();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    sharedMemoryBufferWillReturnFdAndSize();
    getSharedMemoryShouldSucceed();
    getShmBufferShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldSetMaxPlaybacks)
{
    createPlaybackServiceShouldSuccess();
    triggerSetMaxPlaybacks();
    getMaxPlaybacksShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldSetMaxWebAudioPlayers)
{
    createPlaybackServiceShouldSuccess();
    triggerSetMaxWebAudioPlayers();
    getMaxWebAudioPlayersShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldSetClientDisplayName)
{
    createPlaybackServiceShouldSuccess();
    triggerSetClientDisplayName();
    clientDisplayNameShouldBeSet();
}

TEST_F(PlaybackServiceTests, shouldPing)
{
    createPlaybackServiceShouldSuccess();
    triggerPing();
}

// ============================================================================
// setPreloadedCapabilities Tests
// ============================================================================

TEST_F(PlaybackServiceTests, shouldSetPreloadedCapabilitiesWithAudioOnly)
{
    createPlaybackServiceShouldSuccess();

    firebolt::rialto::common::AudioDecoderCapabilities audioCapabilities{};
    audioCapabilities.interfaceVersion = "1.0";
    audioCapabilities.schemaVersion = "";
    audioCapabilities.capabilities = {};

    // setPreloadedCapabilities should not throw
    triggerSetPreloadedCapabilities(audioCapabilities, std::nullopt);
}

TEST_F(PlaybackServiceTests, shouldSetPreloadedCapabilitiesWithVideoOnly)
{
    createPlaybackServiceShouldSuccess();

    firebolt::rialto::common::VideoDecoderCapabilities videoCapabilities{};
    videoCapabilities.interfaceVersion = "2.0";
    videoCapabilities.schemaVersion = "";
    videoCapabilities.capabilities = {};

    // setPreloadedCapabilities should not throw
    triggerSetPreloadedCapabilities(std::nullopt, videoCapabilities);
}

TEST_F(PlaybackServiceTests, shouldSetPreloadedCapabilitiesWithBothAudioAndVideo)
{
    createPlaybackServiceShouldSuccess();

    firebolt::rialto::common::AudioDecoderCapabilities audioCapabilities{};
    audioCapabilities.interfaceVersion = "1.0";
    audioCapabilities.schemaVersion = "2.0";
    audioCapabilities.capabilities = {};

    firebolt::rialto::common::VideoDecoderCapabilities videoCapabilities{};
    videoCapabilities.interfaceVersion = "3.0";
    videoCapabilities.schemaVersion = "4.0";
    videoCapabilities.capabilities = {};

    // setPreloadedCapabilities should not throw
    triggerSetPreloadedCapabilities(audioCapabilities, videoCapabilities);
}

TEST_F(PlaybackServiceTests, shouldSetPreloadedCapabilitiesWithNullopt)
{
    createPlaybackServiceShouldSuccess();

    // setPreloadedCapabilities with both as std::nullopt should not throw
    triggerSetPreloadedCapabilities(std::nullopt, std::nullopt);
}

TEST_F(PlaybackServiceTests, shouldAllowMultipleSetPreloadedCapabilitiesCalls)
{
    createPlaybackServiceShouldSuccess();

    firebolt::rialto::common::AudioDecoderCapabilities audioCapabilities1{};
    audioCapabilities1.interfaceVersion = "1.0";
    audioCapabilities1.schemaVersion = "";
    audioCapabilities1.capabilities = {};

    firebolt::rialto::common::VideoDecoderCapabilities videoCapabilities1{};
    videoCapabilities1.interfaceVersion = "2.0";
    videoCapabilities1.schemaVersion = "";
    videoCapabilities1.capabilities = {};

    // First call
    triggerSetPreloadedCapabilities(audioCapabilities1, videoCapabilities1);

    // Second call with different capabilities
    firebolt::rialto::common::AudioDecoderCapabilities audioCapabilities2{};
    audioCapabilities2.interfaceVersion = "2.0";
    audioCapabilities2.schemaVersion = "";
    audioCapabilities2.capabilities = {};

    firebolt::rialto::common::VideoDecoderCapabilities videoCapabilities2{};
    videoCapabilities2.interfaceVersion = "3.0";
    videoCapabilities2.schemaVersion = "";
    videoCapabilities2.capabilities = {};

    // Should be able to set multiple times
    triggerSetPreloadedCapabilities(audioCapabilities2, videoCapabilities2);
}
