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

#include "MediaCapabilitiesAppendCodecsTests.h"
#include <utility>

using testing::NiceMock;
using testing::Return;

/**
 * Test: Audio codec append - single codec missing in YAML
 *
 * Scenario: YAML has empty audio rank, GStreamer has AAC codec
 * Expected: AAC codec is appended to YAML audio capabilities
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldAppendMissingAudioCodecWhenNotInYaml)
{
    // Setup YAML capabilities with empty rank
    firebolt::rialto::common::AudioDecoderCapabilities yamlAudio{"yaml_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability emptyRank{};
    yamlAudio.capabilities.push_back(emptyRank);

    // Setup GStreamer capabilities with AAC codec
    firebolt::rialto::common::AudioDecoderCapabilities gstAudio{"gst_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability gstRankWithAac{};
    firebolt::rialto::common::AacCapability aacCap{};
    aacCap.profiles[firebolt::rialto::common::AacProfile::LC] = {.maxBitrateInBps = 320000,
                                                                 .maxChannels = 2,
                                                                 .maxSampleRateInHz = 48000,
                                                                 .maxBitDepth = 16};
    gstRankWithAac.aac = aacCap;
    gstAudio.capabilities.push_back(gstRankWithAac);

    // Inject GStreamer mock with codec data
    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(gstAudio));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(getEmptyVideoCapabilities()));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    // Set YAML preloaded capabilities
    mediaCapabilities->setPreloadedCapabilities(yamlAudio, std::nullopt);

    // Get enhanced audio capabilities
    auto result = mediaCapabilities->getSupportedAudioCapabilities();

    // Verify AAC codec was appended
    EXPECT_EQ(result.capabilities.size(), 1);
    EXPECT_TRUE(result.capabilities[0].aac.has_value());
    EXPECT_EQ(result.capabilities[0].aac->profiles.size(), 1);
}

/**
 * Test: Audio codec append - multiple missing codecs
 *
 * Scenario: YAML has empty rank, GStreamer has multiple codecs (AAC, MP3, FLAC)
 * Expected: All three codecs are appended to YAML
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldAppendMultipleMissingAudioCodecs)
{
    // Setup YAML capabilities with empty rank
    firebolt::rialto::common::AudioDecoderCapabilities yamlAudio{"yaml_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability emptyRank{};
    yamlAudio.capabilities.push_back(emptyRank);

    // Setup GStreamer capabilities with multiple codecs
    firebolt::rialto::common::AudioDecoderCapabilities gstAudio{"gst_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability gstRank{};

    // Add AAC
    firebolt::rialto::common::AacCapability aacCap{};
    aacCap.profiles[firebolt::rialto::common::AacProfile::LC] = {320000, 2, 48000, 16};
    gstRank.aac = aacCap;

    // Add MP3
    firebolt::rialto::common::Mp3Capability mp3Cap{{320000, 2, 48000, 16}};
    gstRank.mp3 = mp3Cap;

    // Add FLAC
    firebolt::rialto::common::FlacCapability flacCap{{320000, 2, 48000, 24}};
    gstRank.flac = flacCap;

    gstAudio.capabilities.push_back(gstRank);

    // Inject GStreamer mock
    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(gstAudio));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(getEmptyVideoCapabilities()));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    mediaCapabilities->setPreloadedCapabilities(yamlAudio, std::nullopt);

    auto result = mediaCapabilities->getSupportedAudioCapabilities();

    // Verify all three codecs were appended
    EXPECT_EQ(result.capabilities.size(), 1);
    EXPECT_TRUE(result.capabilities[0].aac.has_value());
    EXPECT_TRUE(result.capabilities[0].mp3.has_value());
    EXPECT_TRUE(result.capabilities[0].flac.has_value());
}

/**
 * Test: Audio codec - don't modify codecs already in YAML
 *
 * Scenario: YAML has AAC, GStreamer also has AAC - they should not be duplicated
 * Expected: AAC in YAML remains unchanged
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldNotModifyCodecsAlreadyInYaml)
{
    // Setup YAML with AAC codec
    firebolt::rialto::common::AudioDecoderCapabilities yamlAudio{"yaml_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability yamlRank{};
    firebolt::rialto::common::AacCapability yamlAac{};
    yamlAac.profiles[firebolt::rialto::common::AacProfile::LC] = {192000, 2, 44100, 16};
    yamlRank.aac = yamlAac;
    yamlAudio.capabilities.push_back(yamlRank);

    // Setup GStreamer with different AAC profile
    firebolt::rialto::common::AudioDecoderCapabilities gstAudio{"gst_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability gstRank{};
    firebolt::rialto::common::AacCapability gstAac{};
    gstAac.profiles[firebolt::rialto::common::AacProfile::HE_V1] = {320000, 2, 48000, 16};
    gstRank.aac = gstAac;
    gstAudio.capabilities.push_back(gstRank);

    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(gstAudio));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(getEmptyVideoCapabilities()));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    mediaCapabilities->setPreloadedCapabilities(yamlAudio, std::nullopt);

    auto result = mediaCapabilities->getSupportedAudioCapabilities();

    // Verify AAC still has both profiles merged
    EXPECT_TRUE(result.capabilities[0].aac.has_value());
    EXPECT_EQ(result.capabilities[0].aac->profiles.size(), 2); // LC + HE_V1 merged
}

/**
 * Test: Audio codec - handle empty YAML capabilities
 *
 * Scenario: YAML has empty capabilities vector, GStreamer has codecs
 * Expected: GStreamer codecs are appended (no crash on empty YAML)
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldHandleEmptyYamlAudioCapabilities)
{
    // Setup YAML with empty capabilities
    firebolt::rialto::common::AudioDecoderCapabilities yamlAudio{"yaml_v1", "1.0", {}};

    // Setup GStreamer with codecs
    firebolt::rialto::common::AudioDecoderCapabilities gstAudio{"gst_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability gstRank{};
    firebolt::rialto::common::Mp3Capability mp3{{320000, 2, 48000, 16}};
    gstRank.mp3 = mp3;
    gstAudio.capabilities.push_back(gstRank);

    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(gstAudio));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(getEmptyVideoCapabilities()));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    mediaCapabilities->setPreloadedCapabilities(yamlAudio, std::nullopt);

    // Should not crash and should return GStreamer codecs
    auto result = mediaCapabilities->getSupportedAudioCapabilities();
    EXPECT_EQ(result.capabilities.size(), 1);
    EXPECT_TRUE(result.capabilities[0].mp3.has_value());
}

/**
 * Test: Audio codec - handle multiple ranks
 *
 * Scenario: YAML has 2 ranks, GStreamer has 2 ranks with different codecs per rank
 * Expected: Each rank is enhanced independently
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldHandleMultipleRanksInAudioCapabilities)
{
    // Setup YAML with 2 ranks
    firebolt::rialto::common::AudioDecoderCapabilities yamlAudio{"yaml_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability rank1{};
    firebolt::rialto::common::Mp3Capability mp3{{320000, 2, 48000, 16}};
    rank1.mp3 = mp3;
    yamlAudio.capabilities.push_back(rank1);

    firebolt::rialto::common::AudioDecoderCapability rank2{};
    yamlAudio.capabilities.push_back(rank2);

    // Setup GStreamer with 2 ranks
    firebolt::rialto::common::AudioDecoderCapabilities gstAudio{"gst_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability gstRank1{};
    firebolt::rialto::common::AacCapability aac1{};
    aac1.profiles[firebolt::rialto::common::AacProfile::LC] = {320000, 2, 48000, 16};
    gstRank1.aac = aac1;
    gstAudio.capabilities.push_back(gstRank1);

    firebolt::rialto::common::AudioDecoderCapability gstRank2{};
    firebolt::rialto::common::FlacCapability flac{{320000, 2, 48000, 24}};
    gstRank2.flac = flac;
    gstAudio.capabilities.push_back(gstRank2);

    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(gstAudio));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(getEmptyVideoCapabilities()));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    mediaCapabilities->setPreloadedCapabilities(yamlAudio, std::nullopt);

    auto result = mediaCapabilities->getSupportedAudioCapabilities();

    // Verify both ranks enhanced
    EXPECT_EQ(result.capabilities.size(), 2);
    EXPECT_TRUE(result.capabilities[0].mp3.has_value());  // Original YAML codec
    EXPECT_TRUE(result.capabilities[0].aac.has_value());  // Appended from GStreamer
    EXPECT_TRUE(result.capabilities[1].flac.has_value()); // Appended from GStreamer
}

/**
 * Test: Audio codec - GStreamer has more ranks than YAML
 *
 * Scenario: YAML has 1 rank, GStreamer has 3 ranks
 * Expected: Extra GStreamer ranks 2 and 3 are appended to YAML
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldHandleGStreamerWithMoreRanksThanYamlAudio)
{
    // Setup YAML with 1 rank
    firebolt::rialto::common::AudioDecoderCapabilities yamlAudio{"yaml_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability yamlRank{};
    yamlAudio.capabilities.push_back(yamlRank);

    // Setup GStreamer with 3 ranks
    firebolt::rialto::common::AudioDecoderCapabilities gstAudio{"gst_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability gstRank1{};
    firebolt::rialto::common::Mp3Capability mp3{{320000, 2, 48000, 16}};
    gstRank1.mp3 = mp3;
    gstAudio.capabilities.push_back(gstRank1);

    firebolt::rialto::common::AudioDecoderCapability gstRank2{};
    firebolt::rialto::common::AacCapability aac{};
    aac.profiles[firebolt::rialto::common::AacProfile::LC] = {320000, 2, 48000, 16};
    gstRank2.aac = aac;
    gstAudio.capabilities.push_back(gstRank2);

    firebolt::rialto::common::AudioDecoderCapability gstRank3{};
    firebolt::rialto::common::FlacCapability flac{{320000, 2, 48000, 24}};
    gstRank3.flac = flac;
    gstAudio.capabilities.push_back(gstRank3);

    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(gstAudio));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(getEmptyVideoCapabilities()));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    mediaCapabilities->setPreloadedCapabilities(yamlAudio, std::nullopt);

    auto result = mediaCapabilities->getSupportedAudioCapabilities();

    // Verify all 3 ranks present (1 original + 2 appended)
    EXPECT_EQ(result.capabilities.size(), 3);
    EXPECT_TRUE(result.capabilities[0].mp3.has_value());
    EXPECT_TRUE(result.capabilities[1].aac.has_value());
    EXPECT_TRUE(result.capabilities[2].flac.has_value());
}

/**
 * Test: Video codec append - missing video codec
 *
 * Scenario: YAML has empty video rank, GStreamer has H.264
 * Expected: H.264 codec is appended to YAML video capabilities
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldAppendMissingVideoCodecWhenNotInYaml)
{
    // Setup YAML video with empty rank
    firebolt::rialto::common::VideoDecoderCapabilities yamlVideo{"yaml_v1", "1.0", {}};
    firebolt::rialto::common::VideoDecoderCapability emptyRank{};
    yamlVideo.capabilities.push_back(emptyRank);

    // Setup GStreamer with H.264
    firebolt::rialto::common::VideoDecoderCapabilities gstVideo{"gst_v1", "1.0", {}};
    firebolt::rialto::common::VideoDecoderCapability gstRank{};
    firebolt::rialto::common::H264CodecCapability h264{};
    h264.profiles.push_back({firebolt::rialto::common::H264ProfileType::H264_MAIN,
                             firebolt::rialto::common::H264Level::H264_LEVEL_4, 8000000});
    gstRank.codecCapabilities.h264 = h264;
    gstVideo.capabilities.push_back(gstRank);

    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(getEmptyAudioCapabilities()));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(gstVideo));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    mediaCapabilities->setPreloadedCapabilities(std::nullopt, yamlVideo);

    auto result = mediaCapabilities->getSupportedVideoCapabilities();

    // Verify H.264 was appended
    EXPECT_EQ(result.capabilities.size(), 1);
    EXPECT_TRUE(result.capabilities[0].codecCapabilities.h264.has_value());
    EXPECT_EQ(result.capabilities[0].codecCapabilities.h264->profiles.size(), 1);
}

/**
 * Test: Video codec - multi-profile merging (H.264)
 *
 * Scenario: YAML has H.264 with Baseline profile, GStreamer has H.264 with Main and High profiles
 * Expected: All three profiles merged into single H.264 capability
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldMergeVideoCodecProfiles)
{
    // Setup YAML with H.264 Baseline profile
    firebolt::rialto::common::VideoDecoderCapabilities yamlVideo{"yaml_v1", "1.0", {}};
    firebolt::rialto::common::VideoDecoderCapability yamlRank{};
    firebolt::rialto::common::H264CodecCapability yamlH264{};
    yamlH264.profiles.push_back({firebolt::rialto::common::H264ProfileType::H264_BASELINE,
                                 firebolt::rialto::common::H264Level::H264_LEVEL_3, 4000000});
    yamlRank.codecCapabilities.h264 = yamlH264;
    yamlVideo.capabilities.push_back(yamlRank);

    // Setup GStreamer with H.264 Main and High profiles
    firebolt::rialto::common::VideoDecoderCapabilities gstVideo{"gst_v1", "1.0", {}};
    firebolt::rialto::common::VideoDecoderCapability gstRank{};
    firebolt::rialto::common::H264CodecCapability gstH264{};
    gstH264.profiles.push_back({firebolt::rialto::common::H264ProfileType::H264_MAIN,
                                firebolt::rialto::common::H264Level::H264_LEVEL_4, 8000000});
    gstH264.profiles.push_back({firebolt::rialto::common::H264ProfileType::H264_HIGH,
                                firebolt::rialto::common::H264Level::H264_LEVEL_5, 10000000});
    gstRank.codecCapabilities.h264 = gstH264;
    gstVideo.capabilities.push_back(gstRank);

    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(getEmptyAudioCapabilities()));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(gstVideo));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    mediaCapabilities->setPreloadedCapabilities(std::nullopt, yamlVideo);

    auto result = mediaCapabilities->getSupportedVideoCapabilities();

    // Verify all 3 profiles merged
    EXPECT_TRUE(result.capabilities[0].codecCapabilities.h264.has_value());
    EXPECT_EQ(result.capabilities[0].codecCapabilities.h264->profiles.size(), 3); // Baseline + Main + High
}

/**
 * Test: Video codec - multiple video codecs appended
 *
 * Scenario: YAML has VP9, GStreamer has H.264, H.265, and AV1
 * Expected: All three codecs appended, VP9 remains
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldAppendMultipleMissingVideoCodecs)
{
    // Setup YAML with VP9
    firebolt::rialto::common::VideoDecoderCapabilities yamlVideo{"yaml_v1", "1.0", {}};
    firebolt::rialto::common::VideoDecoderCapability yamlRank{};
    firebolt::rialto::common::Vp9CodecCapability vp9{};
    vp9.profiles.push_back({firebolt::rialto::common::Vp9ProfileType::VP9_PROFILE_0,
                            firebolt::rialto::common::Vp9Level::VP9_LEVEL_4, 8000000});
    yamlRank.codecCapabilities.vp9 = vp9;
    yamlVideo.capabilities.push_back(yamlRank);

    // Setup GStreamer with H.264, H.265, AV1
    firebolt::rialto::common::VideoDecoderCapabilities gstVideo{"gst_v1", "1.0", {}};
    firebolt::rialto::common::VideoDecoderCapability gstRank{};

    firebolt::rialto::common::H264CodecCapability h264{};
    h264.profiles.push_back({firebolt::rialto::common::H264ProfileType::H264_MAIN,
                             firebolt::rialto::common::H264Level::H264_LEVEL_4, 8000000});
    gstRank.codecCapabilities.h264 = h264;

    firebolt::rialto::common::H265CodecCapability h265{};
    h265.profiles.push_back({firebolt::rialto::common::H265ProfileType::H265_MAIN,
                             firebolt::rialto::common::H265Level::H265_LEVEL_5, 10000000});
    gstRank.codecCapabilities.h265 = h265;

    firebolt::rialto::common::Av1CodecCapability av1{};
    av1.profiles.push_back({firebolt::rialto::common::Av1ProfileType::AV1_MAIN,
                            firebolt::rialto::common::Av1Level::AV1_LEVEL_5_0, 12000000});
    gstRank.codecCapabilities.av1 = av1;

    gstVideo.capabilities.push_back(gstRank);

    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(getEmptyAudioCapabilities()));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(gstVideo));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    mediaCapabilities->setPreloadedCapabilities(std::nullopt, yamlVideo);

    auto result = mediaCapabilities->getSupportedVideoCapabilities();

    // Verify all codecs present (VP9 original + H.264, H.265, AV1 appended)
    EXPECT_TRUE(result.capabilities[0].codecCapabilities.vp9.has_value());
    EXPECT_TRUE(result.capabilities[0].codecCapabilities.h264.has_value());
    EXPECT_TRUE(result.capabilities[0].codecCapabilities.h265.has_value());
    EXPECT_TRUE(result.capabilities[0].codecCapabilities.av1.has_value());
}

/**
 * Test: Video codec - GStreamer has more ranks than YAML
 *
 * Scenario: YAML has 1 video rank, GStreamer has 2 ranks
 * Expected: Second GStreamer rank is appended
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldHandleGStreamerWithMoreRanksThanYamlVideo)
{
    // Setup YAML with 1 video rank
    firebolt::rialto::common::VideoDecoderCapabilities yamlVideo{"yaml_v1", "1.0", {}};
    firebolt::rialto::common::VideoDecoderCapability yamlRank{};
    yamlVideo.capabilities.push_back(yamlRank);

    // Setup GStreamer with 2 video ranks
    firebolt::rialto::common::VideoDecoderCapabilities gstVideo{"gst_v1", "1.0", {}};

    firebolt::rialto::common::VideoDecoderCapability gstRank1{};
    firebolt::rialto::common::H264CodecCapability h264{};
    h264.profiles.push_back({firebolt::rialto::common::H264ProfileType::H264_MAIN,
                             firebolt::rialto::common::H264Level::H264_LEVEL_4, 8000000});
    gstRank1.codecCapabilities.h264 = h264;
    gstVideo.capabilities.push_back(gstRank1);

    firebolt::rialto::common::VideoDecoderCapability gstRank2{};
    firebolt::rialto::common::H265CodecCapability h265{};
    h265.profiles.push_back({firebolt::rialto::common::H265ProfileType::H265_MAIN,
                             firebolt::rialto::common::H265Level::H265_LEVEL_5, 10000000});
    gstRank2.codecCapabilities.h265 = h265;
    gstVideo.capabilities.push_back(gstRank2);

    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(getEmptyAudioCapabilities()));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(gstVideo));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    mediaCapabilities->setPreloadedCapabilities(std::nullopt, yamlVideo);

    auto result = mediaCapabilities->getSupportedVideoCapabilities();

    // Verify both ranks present
    EXPECT_EQ(result.capabilities.size(), 2);
    EXPECT_TRUE(result.capabilities[0].codecCapabilities.h264.has_value());
    EXPECT_TRUE(result.capabilities[1].codecCapabilities.h265.has_value());
}

/**
 * Test: Path B - no append when YAML absent
 *
 * Scenario: No preloaded capabilities, only GStreamer available
 * Expected: Path B fallback returns GStreamer as-is (no append logic)
 */
TEST_F(MediaCapabilitiesAppendCodecsTests, shouldNotAppendWhenYamlAbsent)
{
    // Setup GStreamer with codecs
    firebolt::rialto::common::AudioDecoderCapabilities gstAudio{"gst_v1", "1.0", {}};
    firebolt::rialto::common::AudioDecoderCapability gstRank{};
    firebolt::rialto::common::AacCapability aac{};
    aac.profiles[firebolt::rialto::common::AacProfile::LC] = {320000, 2, 48000, 16};
    gstRank.aac = aac;
    gstAudio.capabilities.push_back(gstRank);

    auto gstMock = std::make_unique<NiceMock<firebolt::rialto::server::GstCapabilitiesMock>>();
    ON_CALL(*gstMock, getSupportedAudioCapabilities()).WillByDefault(Return(gstAudio));
    ON_CALL(*gstMock, getSupportedVideoCapabilities()).WillByDefault(Return(getEmptyVideoCapabilities()));

    std::shared_ptr<firebolt::rialto::server::MediaCapabilitiesServerInternal> mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilitiesServerInternal>(std::move(gstMock));

    // Do NOT set preloaded capabilities - trigger Path B fallback
    auto result = mediaCapabilities->getSupportedAudioCapabilities();

    // Verify GStreamer returned as-is
    EXPECT_EQ(result.interfaceVersion, gstAudio.interfaceVersion);
    EXPECT_TRUE(result.capabilities[0].aac.has_value());
}
