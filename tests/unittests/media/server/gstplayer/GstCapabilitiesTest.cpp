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

/**
 * @file GstCapabilitiesTest.cpp
 * @brief Unit tests for GstCapabilities class
 *
 * Purpose: Test coverage for fillSupportedCapabilities() and related methods
 * which were rewritten to populate codec profile data.
 *
 * Coverage Target: +5-7% improvement
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "GstCapabilities.h"
#include "GstCapabilitiesFactory.h"
#include "GstCapabilitiesMock.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::StrictMock;

namespace firebolt::rialto::server
{

/**
 * @class GstCapabilitiesTest
 * @brief Test fixture for GstCapabilities
 *
 * Tests the capability discovery and population from GStreamer
 * including all codec types and profile data.
 */
class GstCapabilitiesTest : public ::testing::Test
{
public:
    GstCapabilitiesTest() = default;
    virtual ~GstCapabilitiesTest() = default;

protected:
    void SetUp() override
    {
        // Create instance of GstCapabilities
        // Note: Actual implementation depends on your factory
    }

    void TearDown() override
    {
        // Cleanup
    }

    // Helper methods
    AudioDecoderCapabilities createDefaultAudioCapabilities()
    {
        AudioDecoderCapabilities caps;
        return caps;
    }

    VideoDecoderCapabilities createDefaultVideoCapabilities()
    {
        VideoDecoderCapabilities caps;
        return caps;
    }
};

/**
 * @test fillSupportedCapabilitiesShouldPopulateAudioCodecs
 * @brief Verify audio codec capabilities are populated with profile data
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateAudioCodecs)
{
    // Create GstCapabilities instance
    // Assuming factory method available
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();

    // Get supported audio capabilities
    auto audioCaps = gstCaps->getSupportedAudioCapabilities();

    // Verify Opus is present and has correct profile
    EXPECT_TRUE(audioCaps.opus.has_value());
    if (audioCaps.opus.has_value())
    {
        auto &opusProfile = audioCaps.opus->audioProfile;
        EXPECT_GT(opusProfile.bitrate, 0);        // Should have bitrate
        EXPECT_EQ(opusProfile.channels, 2);       // Opus typically 2ch
        EXPECT_EQ(opusProfile.sampleRate, 48000); // Opus uses 48kHz
        EXPECT_EQ(opusProfile.bitdepth, 16);      // Typically 16-bit
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulateFLACCapabilities
 * @brief Verify FLAC codec capabilities
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateFLACCapabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto audioCaps = gstCaps->getSupportedAudioCapabilities();

    // Verify FLAC is present with correct profile
    EXPECT_TRUE(audioCaps.flac.has_value());
    if (audioCaps.flac.has_value())
    {
        auto &flacProfile = audioCaps.flac->audioProfile;
        EXPECT_GT(flacProfile.bitrate, 0);
        EXPECT_LE(flacProfile.channels, 8);        // FLAC supports up to 8 channels
        EXPECT_EQ(flacProfile.sampleRate, 192000); // High sample rate
        EXPECT_EQ(flacProfile.bitdepth, 24);       // 24-bit
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulateAACCapabilities
 * @brief Verify AAC codec capabilities with LC profile
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateAACCapabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto audioCaps = gstCaps->getSupportedAudioCapabilities();

    // Verify AAC is present
    EXPECT_TRUE(audioCaps.aac.has_value());
    if (audioCaps.aac.has_value())
    {
        // AAC should have LC profile populated
        EXPECT_FALSE(audioCaps.aac->profiles.empty());

        // Check for LC profile
        bool hasLCProfile = false;
        for (const auto &profile : audioCaps.aac->profiles)
        {
            if (profile.first == "LC") // LC profile
            {
                hasLCProfile = true;
                EXPECT_GT(profile.second.bitrate, 0);
                EXPECT_EQ(profile.second.channels, 2);
            }
        }
        EXPECT_TRUE(hasLCProfile);
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulateMP3Capabilities
 * @brief Verify MP3 codec capabilities
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateMP3Capabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto audioCaps = gstCaps->getSupportedAudioCapabilities();

    // Verify MP3 is present
    EXPECT_TRUE(audioCaps.mp3.has_value());
    if (audioCaps.mp3.has_value())
    {
        auto &mp3Profile = audioCaps.mp3->audioProfile;
        EXPECT_GT(mp3Profile.bitrate, 0);
        EXPECT_LE(mp3Profile.channels, 2); // MP3 typically mono or stereo
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulateEAC3Capabilities
 * @brief Verify E-AC3 codec capabilities with 6-channel support
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateEAC3Capabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto audioCaps = gstCaps->getSupportedAudioCapabilities();

    // Verify EAC3 is present
    EXPECT_TRUE(audioCaps.eac3.has_value());
    if (audioCaps.eac3.has_value())
    {
        auto &eac3Profile = audioCaps.eac3->audioProfile;
        EXPECT_GT(eac3Profile.bitrate, 0);
        EXPECT_EQ(eac3Profile.channels, 6); // 5.1 channels
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulateAC3Capabilities
 * @brief Verify AC3 codec capabilities with 6-channel support
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateAC3Capabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto audioCaps = gstCaps->getSupportedAudioCapabilities();

    // Verify AC3 is present
    EXPECT_TRUE(audioCaps.ac3.has_value());
    if (audioCaps.ac3.has_value())
    {
        auto &ac3Profile = audioCaps.ac3->audioProfile;
        EXPECT_GT(ac3Profile.bitrate, 0);
        EXPECT_EQ(ac3Profile.channels, 6); // 5.1 channels
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulateVorbisCapabilities
 * @brief Verify Vorbis codec capabilities
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateVorbisCapabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto audioCaps = gstCaps->getSupportedAudioCapabilities();

    // Verify Vorbis is present
    EXPECT_TRUE(audioCaps.vorbis.has_value());
    if (audioCaps.vorbis.has_value())
    {
        auto &vorbisProfile = audioCaps.vorbis->audioProfile;
        EXPECT_GT(vorbisProfile.bitrate, 0);
        EXPECT_LE(vorbisProfile.channels, 2); // Vorbis typically stereo
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulatePCMCapabilities
 * @brief Verify PCM fallback codec capabilities
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulatePCMCapabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto audioCaps = gstCaps->getSupportedAudioCapabilities();

    // PCM should be fallback
    EXPECT_TRUE(audioCaps.pcm.has_value());
    if (audioCaps.pcm.has_value())
    {
        auto &pcmProfile = audioCaps.pcm->audioProfile;
        EXPECT_GT(pcmProfile.bitrate, 0);
    }
}

// ============================================================================
// VIDEO CODEC TESTS
// ============================================================================

/**
 * @test fillSupportedCapabilitiesShouldPopulateH264Capabilities
 * @brief Verify H.264 codec capabilities with Main profile and level 5.2
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateH264Capabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto videoCaps = gstCaps->getSupportedVideoCapabilities();

    // Verify H.264 is present
    EXPECT_TRUE(videoCaps.h264.has_value());
    if (videoCaps.h264.has_value())
    {
        // Should have Main profile at least
        EXPECT_FALSE(videoCaps.h264->profiles.empty());

        bool hasMainProfile = false;
        for (const auto &profile : videoCaps.h264->profiles)
        {
            if (profile.first == "Main")
            {
                hasMainProfile = true;
                // Check level 5.2
                EXPECT_GE(profile.second.level, 5.2);
            }
        }
        EXPECT_TRUE(hasMainProfile);
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulateHEVCCapabilities
 * @brief Verify H.265/HEVC codec capabilities
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateHEVCCapabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto videoCaps = gstCaps->getSupportedVideoCapabilities();

    // Verify H.265/HEVC is present
    EXPECT_TRUE(videoCaps.h265.has_value());
    if (videoCaps.h265.has_value())
    {
        // Should have Main profile at least
        EXPECT_FALSE(videoCaps.h265->profiles.empty());

        bool hasMainProfile = false;
        for (const auto &profile : videoCaps.h265->profiles)
        {
            if (profile.first == "Main")
            {
                hasMainProfile = true;
                EXPECT_GE(profile.second.level, 5.2);
            }
        }
        EXPECT_TRUE(hasMainProfile);
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulateVP9Capabilities
 * @brief Verify VP9 codec capabilities with Profile 0 and Level 5.2
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateVP9Capabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto videoCaps = gstCaps->getSupportedVideoCapabilities();

    // Verify VP9 is present
    EXPECT_TRUE(videoCaps.vp9.has_value());
    if (videoCaps.vp9.has_value())
    {
        // Should have Profile0 at least
        EXPECT_FALSE(videoCaps.vp9->profiles.empty());

        bool hasProfile0 = false;
        for (const auto &profile : videoCaps.vp9->profiles)
        {
            if (profile.first == "Profile0")
            {
                hasProfile0 = true;
                EXPECT_GE(profile.second.level, 5.2);
            }
        }
        EXPECT_TRUE(hasProfile0);
    }
}

/**
 * @test fillSupportedCapabilitiesShouldPopulateAV1Capabilities
 * @brief Verify AV1 codec capabilities with Main profile and Level 6.2
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldPopulateAV1Capabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto videoCaps = gstCaps->getSupportedVideoCapabilities();

    // Verify AV1 is present
    EXPECT_TRUE(videoCaps.av1.has_value());
    if (videoCaps.av1.has_value())
    {
        // Should have Main profile at least
        EXPECT_FALSE(videoCaps.av1->profiles.empty());

        bool hasMainProfile = false;
        for (const auto &profile : videoCaps.av1->profiles)
        {
            if (profile.first == "Main")
            {
                hasMainProfile = true;
                EXPECT_GE(profile.second.level, 6.2);
            }
        }
        EXPECT_TRUE(hasMainProfile);
    }
}

/**
 * @test fillSupportedCapabilitiesShouldHaveFallbackH264
 * @brief Verify H.264 fallback is available
 */
TEST_F(GstCapabilitiesTest, fillSupportedCapabilitiesShouldHaveFallbackH264)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto videoCaps = gstCaps->getSupportedVideoCapabilities();

    // H.264 should be fallback
    EXPECT_TRUE(videoCaps.h264.has_value());
}

// ============================================================================
// EDGE CASE AND ERROR CONDITION TESTS
// ============================================================================

/**
 * @test getSupportedAudioCapabilitiesShouldReturnValidCapabilities
 * @brief Verify audio capabilities are returned in valid state
 */
TEST_F(GstCapabilitiesTest, getSupportedAudioCapabilitiesShouldReturnValidCapabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto audioCaps = gstCaps->getSupportedAudioCapabilities();

    // Should have at least one codec
    bool hasAtLeastOneCodec = audioCaps.opus.has_value() || audioCaps.flac.has_value() || audioCaps.aac.has_value() ||
                              audioCaps.mp3.has_value() || audioCaps.eac3.has_value() || audioCaps.ac3.has_value() ||
                              audioCaps.vorbis.has_value() || audioCaps.pcm.has_value();

    EXPECT_TRUE(hasAtLeastOneCodec);
}

/**
 * @test getSupportedVideoCapabilitiesShouldReturnValidCapabilities
 * @brief Verify video capabilities are returned in valid state
 */
TEST_F(GstCapabilitiesTest, getSupportedVideoCapabilitiesShouldReturnValidCapabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();
    auto videoCaps = gstCaps->getSupportedVideoCapabilities();

    // Should have at least one codec
    bool hasAtLeastOneCodec = videoCaps.h264.has_value() || videoCaps.h265.has_value() || videoCaps.vp9.has_value() ||
                              videoCaps.av1.has_value();

    EXPECT_TRUE(hasAtLeastOneCodec);
}

/**
 * @test multipleCallsShouldReturnConsistentCapabilities
 * @brief Verify multiple calls return consistent results (no variation)
 */
TEST_F(GstCapabilitiesTest, multipleCallsShouldReturnConsistentCapabilities)
{
    auto gstCaps = IGstCapabilitiesFactory::getFactory()->createGstCapabilities();

    auto audioCaps1 = gstCaps->getSupportedAudioCapabilities();
    auto audioCaps2 = gstCaps->getSupportedAudioCapabilities();

    // Both calls should return same codec support
    EXPECT_EQ(audioCaps1.opus.has_value(), audioCaps2.opus.has_value());
    EXPECT_EQ(audioCaps1.aac.has_value(), audioCaps2.aac.has_value());
    EXPECT_EQ(audioCaps1.mp3.has_value(), audioCaps2.mp3.has_value());
}

} // namespace firebolt::rialto::server
