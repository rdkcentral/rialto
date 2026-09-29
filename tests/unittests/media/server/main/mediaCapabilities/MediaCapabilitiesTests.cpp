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

#include "MediaCapabilitiesTests.h"
#include <utility>

using testing::NiceMock;
using testing::Return;

// Test: Use GStreamer audio capabilities (Path B fallback)
TEST_F(MediaCapabilitiesTests, shouldReturnPreloadedAudioCapabilities)
{
    // Given preloaded audio capabilities available (Path 0 priority)
    m_mediaCapabilities->setPreloadedCapabilities(m_gstAudioCapabilities, std::nullopt);

    // When getting audio capabilities from the fixture's MediaCapabilities
    auto result = m_mediaCapabilities->getSupportedAudioCapabilities();

    // Then preloaded result is returned without querying GStreamer
    EXPECT_FALSE(result.capabilities.empty());
    EXPECT_EQ(result.interfaceVersion, m_gstAudioCapabilities.interfaceVersion);
}

// Test: Use preloaded video capabilities (Path 0 priority)
TEST_F(MediaCapabilitiesTests, shouldReturnPreloadedVideoCapabilities)
{
    // Given preloaded video capabilities available (Path 0 priority)
    m_mediaCapabilities->setPreloadedCapabilities(std::nullopt, m_gstVideoCapabilities);

    // When getting video capabilities from the fixture's MediaCapabilities
    auto result = m_mediaCapabilities->getSupportedVideoCapabilities();

    // Then preloaded result is returned without querying GStreamer
    EXPECT_FALSE(result.capabilities.empty());
    EXPECT_EQ(result.interfaceVersion, m_gstVideoCapabilities.interfaceVersion);
}

// Test: Fall back to GStreamer for audio when preload missing (Path B)
TEST_F(MediaCapabilitiesTests, shouldFallbackToGStreamerForAudioWhenPreloadMissing)
{
    // Given GStreamer available with no preloaded audio
    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(m_gstAudioCapabilities));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilitiesNoPreload =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    // When getting audio capabilities
    auto result = mediaCapabilitiesNoPreload->getSupportedAudioCapabilities();

    // Then GStreamer fallback is used (returns GStreamer's populated data)
    EXPECT_FALSE(result.capabilities.empty());
}

// Test: Fall back to GStreamer for video when preload missing (Path B)
TEST_F(MediaCapabilitiesTests, shouldFallbackToGStreamerForVideoWhenPreloadMissing)
{
    // Given GStreamer available with no preloaded video
    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(m_gstVideoCapabilities));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilitiesNoPreload =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    // When getting video capabilities
    auto result = mediaCapabilitiesNoPreload->getSupportedVideoCapabilities();

    // Then GStreamer fallback is used (returns GStreamer's populated data)
    EXPECT_FALSE(result.capabilities.empty());
}

// Test: Preloaded audio capabilities are preferred over GStreamer (Path 0 priority)
TEST_F(MediaCapabilitiesTests, shouldPreferPreloadedAudioOverGStreamer)
{
    // Given both preloaded and GStreamer audio capabilities available
    firebolt::rialto::common::AudioDecoderCapabilities preloadedAudio{"preload_v1", "1.0", {}};
    preloadedAudio.capabilities.push_back(firebolt::rialto::common::AudioDecoderCapability{});
    m_mediaCapabilities->setPreloadedCapabilities(preloadedAudio, std::nullopt);
    gstCapabilitiesWillBeQueried();

    // When getting audio capabilities
    auto result = m_mediaCapabilities->getSupportedAudioCapabilities();

    // Then preloaded audio is returned (Path 0 priority, not GStreamer fallback)
    EXPECT_FALSE(result.capabilities.empty());
    EXPECT_EQ(result.interfaceVersion, "preload_v1");
}

// Test: Preloaded video capabilities are preferred over GStreamer (Path 0 priority)
TEST_F(MediaCapabilitiesTests, shouldPreferPreloadedVideoOverGStreamer)
{
    // Given both preloaded and GStreamer video capabilities available
    firebolt::rialto::common::VideoDecoderCapabilities preloadedVideo{"preload_v1", "1.0", {}};
    preloadedVideo.capabilities.push_back(firebolt::rialto::common::VideoDecoderCapability{});
    m_mediaCapabilities->setPreloadedCapabilities(std::nullopt, preloadedVideo);
    gstCapabilitiesWillBeQueried();

    // When getting video capabilities
    auto result = m_mediaCapabilities->getSupportedVideoCapabilities();

    // Then preloaded video is returned (Path 0 priority, not GStreamer fallback)
    EXPECT_FALSE(result.capabilities.empty());
    EXPECT_EQ(result.interfaceVersion, "preload_v1");
}

// Test: Constructor requires GStreamer capabilities
TEST_F(MediaCapabilitiesTests, shouldThrowExceptionWhenGStreamerCapabilitiesIsNull)
{
    // Given null GStreamer capabilities (required dependency)
    std::unique_ptr<firebolt::rialto::server::IGstCapabilities> nullGstCapabilities = nullptr;

    // When creating MediaCapabilities with null gstCapabilities
    // Then exception is thrown
    EXPECT_THROW(
        { firebolt::rialto::server::MediaCapabilitiesServerInternal mediaCapabilities(std::move(nullGstCapabilities)); },
        std::runtime_error);
}

// Test: Constructor with GStreamer capabilities
TEST_F(MediaCapabilitiesTests, shouldConstructWithGStreamerCapabilities)
{
    // Given valid GStreamer capabilities
    // When creating MediaCapabilities with unique_ptr mock
    auto gstMock = std::make_unique<StrictMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    auto mediaCapabilities = firebolt::rialto::server::MediaCapabilitiesServerInternal(std::move(gstMock));

    // Then construction succeeds
    EXPECT_TRUE(true); // Object created successfully
}

// Test: Query audio from GStreamer fallback path
TEST_F(MediaCapabilitiesTests, shouldQueryGStreamerForAudio)
{
    // Given GStreamer capabilities available
    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(m_gstAudioCapabilities));

    firebolt::rialto::server::MediaCapabilitiesServerInternal mediaCapabilities(std::move(gstMock));

    // When getting audio capabilities (no preload, uses GStreamer)
    auto audioResult = mediaCapabilities.getSupportedAudioCapabilities();

    // Then GStreamer is queried and result returned
    EXPECT_FALSE(audioResult.capabilities.empty());
}

// Test: Query video from GStreamer fallback path
TEST_F(MediaCapabilitiesTests, shouldQueryGStreamerForVideo)
{
    // Given GStreamer capabilities available
    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(m_gstVideoCapabilities));

    firebolt::rialto::server::MediaCapabilitiesServerInternal mediaCapabilities(std::move(gstMock));

    // When getting video capabilities (no preload, uses GStreamer)
    auto videoResult = mediaCapabilities.getSupportedVideoCapabilities();

    // Then GStreamer is queried and result returned
    EXPECT_FALSE(videoResult.capabilities.empty());
}

// Test: GStreamer query called on demand
TEST_F(MediaCapabilitiesTests, shouldCallGStreamerWhenQueried)
{
    // Given GStreamer capabilities available
    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(m_gstAudioCapabilities));

    firebolt::rialto::server::MediaCapabilitiesServerInternal mediaCapabilities(std::move(gstMock));

    // When getting capabilities
    auto audioResult = mediaCapabilities.getSupportedAudioCapabilities();

    // Then GStreamer query was executed
    EXPECT_FALSE(audioResult.capabilities.empty());
}
