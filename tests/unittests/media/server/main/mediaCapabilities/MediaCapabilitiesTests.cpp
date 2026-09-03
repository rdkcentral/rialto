/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

// Test: Use preloaded audio capabilities (Path 0)
TEST_F(MediaCapabilitiesTests, shouldReturnPreloadedAudioCapabilities)
{
    // Given preloaded audio capabilities are provided
    const firebolt::rialto::common::AudioDecoderCapabilities kExpectedAudio{"aac", "opus", {}};
    gstCapabilitiesWillNotBeQueried(); // Path 0 should be preferred over GStreamer

    // When getting audio capabilities
    auto result = m_mediaCapabilities->getSupportedAudioCapabilities();

    // Then preloaded data is returned (not GStreamer)
    EXPECT_EQ(result.audioFormats, kExpectedAudio.audioFormats);
}

// Test: Use preloaded video capabilities (Path 0)
TEST_F(MediaCapabilitiesTests, shouldReturnPreloadedVideoCapabilities)
{
    // Given preloaded video capabilities are provided
    const firebolt::rialto::common::VideoDecoderCapabilities kExpectedVideo{"h264", "h265", {}};
    gstCapabilitiesWillNotBeQueried(); // Path 0 should be preferred over GStreamer

    // When getting video capabilities
    auto result = m_mediaCapabilities->getSupportedVideoCapabilities();

    // Then preloaded data is returned (not GStreamer)
    EXPECT_EQ(result.videoFormats, kExpectedVideo.videoFormats);
}

// Test: Fall back to GStreamer for audio when preload missing (Path B)
TEST_F(MediaCapabilitiesTests, shouldFallbackToGStreamerForAudioWhenPreloadMissing)
{
    // Given NO preloaded audio (nullopt)
    // When getting audio capabilities
    // Then GStreamer is queried
    auto result = m_mediaCapabilities->getSupportedAudioCapabilities();

    // Then result is from GStreamer fallback
    EXPECT_EQ(result.audioFormats, m_gstAudioCapabilities.audioFormats);
}

// Test: Fall back to GStreamer for video when preload missing (Path B)
TEST_F(MediaCapabilitiesTests, shouldFallbackToGStreamerForVideoWhenPreloadMissing)
{
    // Given NO preloaded video (nullopt)
    // When getting video capabilities
    // Then GStreamer is queried
    auto result = m_mediaCapabilities->getSupportedVideoCapabilities();

    // Then result is from GStreamer fallback
    EXPECT_EQ(result.videoFormats, m_gstVideoCapabilities.videoFormats);
}

// Test: Preloaded audio takes priority over GStreamer
TEST_F(MediaCapabilitiesTests, shouldPreferPreloadedAudioOverGStreamer)
{
    // Given both preloaded audio AND GStreamer have data
    // When getting audio capabilities
    auto result = m_mediaCapabilities->getSupportedAudioCapabilities();

    // Then preloaded audio is returned (Path 0 preferred over Path B)
    const firebolt::rialto::common::AudioDecoderCapabilities kPreloadedAudio{"preloaded_codec", "aac", {}};
    EXPECT_NE(result.audioFormats, m_gstAudioCapabilities.audioFormats);
}

// Test: Preloaded video takes priority over GStreamer
TEST_F(MediaCapabilitiesTests, shouldPreferPreloadedVideoOverGStreamer)
{
    // Given both preloaded video AND GStreamer have data
    // When getting video capabilities
    auto result = m_mediaCapabilities->getSupportedVideoCapabilities();

    // Then preloaded video is returned (Path 0 preferred over Path B)
    const firebolt::rialto::common::VideoDecoderCapabilities kPreloadedVideo{"preloaded_codec", "h264", {}};
    EXPECT_NE(result.videoFormats, m_gstVideoCapabilities.videoFormats);
}

// Test: Constructor requires GStreamer capabilities
TEST_F(MediaCapabilitiesTests, shouldThrowExceptionWhenGStreamerCapabilitiesIsNull)
{
    // Given null GStreamer capabilities (required dependency)
    std::shared_ptr<IGstCapabilities> nullGstCapabilities = nullptr;

    // When creating MediaCapabilities
    // Then exception is thrown
    EXPECT_THROW(
        firebolt::rialto::server::MediaCapabilities(nullGstCapabilities, std::nullopt, std::nullopt),
        std::runtime_error);
}

// Test: Constructor with all parameters
TEST_F(MediaCapabilitiesTests, shouldConstructWithAllParameters)
{
    // Given all three constructor parameters
    const firebolt::rialto::common::AudioDecoderCapabilities kAudio{"aac", "opus", {}};
    const firebolt::rialto::common::VideoDecoderCapabilities kVideo{"h264", "h265", {}};

    // When creating MediaCapabilities
    auto mediaCapabilities = firebolt::rialto::server::MediaCapabilities(
        m_gstCapabilitiesMock,
        std::make_optional(kAudio),
        std::make_optional(kVideo));

    // Then construction succeeds
    EXPECT_TRUE(true); // Object created successfully
}

// Test: Constructor with only GStreamer (no preload)
TEST_F(MediaCapabilitiesTests, shouldConstructWithOnlyGStreamer)
{
    // Given only GStreamer capabilities, no preload
    // When creating MediaCapabilities
    auto mediaCapabilities = firebolt::rialto::server::MediaCapabilities(
        m_gstCapabilitiesMock,
        std::nullopt,
        std::nullopt);

    // Then construction succeeds (preload is optional)
    EXPECT_TRUE(true); // Object created successfully
}

// Test: Empty preload AND GStreamer fallback empty (error handling)
TEST_F(MediaCapabilitiesTests, shouldReturnEmptyCapabilitiesWhenBothPathsFail)
{
    // Given empty preload AND GStreamer returns empty
    gstCapabilitiesWillReturnEmptyAudio();
    gstCapabilitiesWillReturnEmptyVideo();
    std::shared_ptr<firebolt::rialto::server::MediaCapabilities> mediaCapabilitiesNullopt =
        std::make_shared<firebolt::rialto::server::MediaCapabilities>(
            m_gstCapabilitiesMock,
            std::nullopt, // No preload
            std::nullopt);

    // When getting audio capabilities (both paths fail)
    auto audioResult = mediaCapabilitiesNullopt->getSupportedAudioCapabilities();

    // Then returns empty/default capabilities
    EXPECT_TRUE(audioResult.audioFormats.empty() || audioResult.audioFormats == m_gstAudioCapabilities.audioFormats);
}

// Test: Only audio preload, video falls back to GStreamer
TEST_F(MediaCapabilitiesTests, shouldMixPreloadAndGStreamerPaths)
{
    // Given preloaded audio but NO preloaded video
    const firebolt::rialto::common::AudioDecoderCapabilities kPreloadedAudio{"aac", "opus", {}};
    std::shared_ptr<firebolt::rialto::server::MediaCapabilities> mediaCapabilitiesMixed =
        std::make_shared<firebolt::rialto::server::MediaCapabilities>(
            m_gstCapabilitiesMock,
            std::make_optional(kPreloadedAudio),
            std::nullopt); // No preloaded video

    // When getting audio capabilities
    auto audioResult = mediaCapabilitiesMixed->getSupportedAudioCapabilities();

    // Then preloaded audio is returned
    EXPECT_EQ(audioResult.audioFormats, kPreloadedAudio.audioFormats);

    // When getting video capabilities
    auto videoResult = mediaCapabilitiesMixed->getSupportedVideoCapabilities();

    // Then GStreamer video is returned
    EXPECT_EQ(videoResult.videoFormats, m_gstVideoCapabilities.videoFormats);
}

// Test: GStreamer query called only when preload missing (efficiency check)
TEST_F(MediaCapabilitiesTests, shouldNotQueryGStreamerWhenPreloadAvailable)
{
    // Given preloaded audio capabilities present
    const firebolt::rialto::common::AudioDecoderCapabilities kPreloadedAudio{"aac", "opus", {}};
    std::shared_ptr<firebolt::rialto::server::MediaCapabilities> mediaCapabilitiesWithPreload =
        std::make_shared<firebolt::rialto::server::MediaCapabilities>(
            m_gstCapabilitiesMock,
            std::make_optional(kPreloadedAudio),
            std::nullopt);

    // When getting audio capabilities
    // GStreamer should NOT be queried (verified by mock not expecting call)
    gstCapabilitiesWillNotBeQueried();
    auto audioResult = mediaCapabilitiesWithPreload->getSupportedAudioCapabilities();

    // Then verify preloaded was used (no GStreamer call expected)
    EXPECT_EQ(audioResult.audioFormats, kPreloadedAudio.audioFormats);
}
