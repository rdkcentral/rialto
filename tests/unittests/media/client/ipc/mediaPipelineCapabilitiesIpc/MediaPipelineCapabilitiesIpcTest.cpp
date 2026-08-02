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

#include "MediaPipelineCapabilitiesIpc.h"
#include "IpcModuleBase.h"
#include "MediaPipelineStructureMatchers.h"
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::WithArgs;

namespace
{
const char *kPropertyName = "immediate-output";
const std::string kInterfaceVersion{"2.0.0"};
const std::string kSchemaVersion{"1.0.0"};
} // namespace
class MediaPipelineCapabilitiesIpcTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<IMediaPipelineCapabilities> m_sut;

    void createMediaPipelineCapabilitiesIpc()
    {
        expectInitIpc();

        EXPECT_NO_THROW(m_sut = std::make_unique<MediaPipelineCapabilitiesIpc>(*m_ipcClientMock));
    }

    std::vector<std::string> m_mimeTypes = {"video/h264", "video/h265"};

public:
    void setGetSupportedMimeTypesResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSupportedMimeTypesResponse *getSupportedMimeTypesResponse =
            dynamic_cast<firebolt::rialto::GetSupportedMimeTypesResponse *>(response);
        for (const std::string &mimeType : m_mimeTypes)
        {
            getSupportedMimeTypesResponse->add_mime_types(mimeType);
        }
    }

    void setIsMimeTypeSupportedResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::IsMimeTypeSupportedResponse *isMimeTypeSupportedResponse =
            dynamic_cast<firebolt::rialto::IsMimeTypeSupportedResponse *>(response);
        isMimeTypeSupportedResponse->set_is_supported(true);
    }

    void setGetSupportedPropertiesResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSupportedPropertiesResponse *getSupportedPropertiesResponse =
            dynamic_cast<firebolt::rialto::GetSupportedPropertiesResponse *>(response);
        getSupportedPropertiesResponse->add_supported_properties(kPropertyName);
    }

    void setIsVideoMasterResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::IsVideoMasterResponse *isVideoMasterResponse =
            dynamic_cast<firebolt::rialto::IsVideoMasterResponse *>(response);
        isVideoMasterResponse->set_is_video_master(true);
    }

    void setGetSupportedAudioCapabilitiesResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSupportedAudioCapabilitiesResponse *getSupportedAudioCapabResp =
            dynamic_cast<firebolt::rialto::GetSupportedAudioCapabilitiesResponse *>(response);
        getSupportedAudioCapabResp->set_interface_version(kInterfaceVersion);
        getSupportedAudioCapabResp->set_schema_version(kSchemaVersion);
        auto *cap = getSupportedAudioCapabResp->add_capabilities();
        cap->mutable_pcm()->mutable_base()->set_max_bitrate_in_bps(1536000);
        cap->mutable_pcm()->mutable_base()->set_max_channels(8);
        cap->mutable_pcm()->mutable_base()->set_max_sample_rate_in_hz(192000);
        cap->mutable_pcm()->mutable_base()->set_max_bit_depth(32);
        auto *aacEntry = cap->mutable_aac()->add_profiles();
        aacEntry->set_profile(firebolt::rialto::GetSupportedAudioCapabilitiesResponse::AAC_PROFILE_LC);
        aacEntry->mutable_capability()->set_max_bitrate_in_bps(576000);
        aacEntry->mutable_capability()->set_max_channels(8);
        aacEntry->mutable_capability()->set_max_sample_rate_in_hz(96000);
        aacEntry->mutable_capability()->set_max_bit_depth(24);
        cap->mutable_mp3()->mutable_base()->set_max_bitrate_in_bps(320000);
        cap->mutable_mp3()->mutable_base()->set_max_channels(2);
        cap->mutable_mp3()->mutable_base()->set_max_sample_rate_in_hz(48000);
        cap->mutable_mp3()->mutable_base()->set_max_bit_depth(16);
        cap->mutable_alac()->mutable_base()->set_max_bitrate_in_bps(1000000);
        cap->mutable_alac()->mutable_base()->set_max_channels(8);
        cap->mutable_alac()->mutable_base()->set_max_sample_rate_in_hz(96000);
        cap->mutable_alac()->mutable_base()->set_max_bit_depth(32);
        cap->mutable_sbc()->mutable_base()->set_max_bitrate_in_bps(320000);
        cap->mutable_sbc()->mutable_base()->set_max_channels(2);
        cap->mutable_sbc()->mutable_base()->set_max_sample_rate_in_hz(44100);
        cap->mutable_sbc()->mutable_base()->set_max_bit_depth(16);
        auto *ac3Entry = cap->mutable_dolby_ac3()->add_profiles();
        ac3Entry->set_profile(firebolt::rialto::GetSupportedAudioCapabilitiesResponse::DOLBY_AC3_PROFILE_STANDARD);
        ac3Entry->mutable_capability()->set_max_bitrate_in_bps(640000);
        ac3Entry->mutable_capability()->set_max_channels(6);
        ac3Entry->mutable_capability()->set_max_sample_rate_in_hz(48000);
        ac3Entry->mutable_capability()->set_max_bit_depth(24);
        cap->mutable_dolby_ac4()->mutable_base()->set_max_bitrate_in_bps(2688000);
        cap->mutable_dolby_ac4()->mutable_base()->set_max_channels(16);
        cap->mutable_dolby_ac4()->mutable_base()->set_max_sample_rate_in_hz(48000);
        cap->mutable_dolby_ac4()->mutable_base()->set_max_bit_depth(24);
        auto *eac3Entry = cap->mutable_dolby_eac3()->add_profiles();
        eac3Entry->set_profile(firebolt::rialto::GetSupportedAudioCapabilitiesResponse::DOLBY_EAC3_PROFILE_PLUS);
        eac3Entry->mutable_capability()->set_max_bitrate_in_bps(6144000);
        eac3Entry->mutable_capability()->set_max_channels(8);
        eac3Entry->mutable_capability()->set_max_sample_rate_in_hz(48000);
        eac3Entry->mutable_capability()->set_max_bit_depth(24);
        cap->mutable_dolby_truehd()->mutable_base()->set_max_bitrate_in_bps(18000000);
        cap->mutable_dolby_truehd()->mutable_base()->set_max_channels(8);
        cap->mutable_dolby_truehd()->mutable_base()->set_max_sample_rate_in_hz(192000);
        cap->mutable_dolby_truehd()->mutable_base()->set_max_bit_depth(24);
        cap->mutable_flac()->mutable_base()->set_max_bitrate_in_bps(1000000);
        cap->mutable_flac()->mutable_base()->set_max_channels(8);
        cap->mutable_flac()->mutable_base()->set_max_sample_rate_in_hz(192000);
        cap->mutable_flac()->mutable_base()->set_max_bit_depth(32);
        cap->mutable_vorbis()->mutable_base()->set_max_bitrate_in_bps(500000);
        cap->mutable_vorbis()->mutable_base()->set_max_channels(8);
        cap->mutable_vorbis()->mutable_base()->set_max_sample_rate_in_hz(48000);
        cap->mutable_vorbis()->mutable_base()->set_max_bit_depth(16);
        cap->mutable_opus()->mutable_base()->set_max_bitrate_in_bps(510000);
        cap->mutable_opus()->mutable_base()->set_max_channels(8);
        cap->mutable_opus()->mutable_base()->set_max_sample_rate_in_hz(48000);
        cap->mutable_opus()->mutable_base()->set_max_bit_depth(16);
    }

    void setGetSupportedVideoCapabilitiesResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSupportedVideoCapabilitiesResponse *getSupportedVideoCapabResp =
            dynamic_cast<firebolt::rialto::GetSupportedVideoCapabilitiesResponse *>(response);
        getSupportedVideoCapabResp->set_interface_version(kInterfaceVersion);
        getSupportedVideoCapabResp->set_schema_version(kSchemaVersion);
        auto *cap = getSupportedVideoCapabResp->add_capabilities();
        auto *h264 = cap->mutable_codec_capabilities()->mutable_h264();
        auto *h264Profile = h264->add_profiles();
        h264Profile->set_type(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::H264_PROFILE_HIGH);
        h264Profile->set_max_level(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::H264_LEVEL_5_1);
        h264Profile->set_max_bitrate_in_bps(50000000);
        h264->add_dynamic_ranges(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::DYNAMIC_RANGE_SDR);
        h264->add_dynamic_ranges(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10);
        auto *h265 = cap->mutable_codec_capabilities()->mutable_h265();
        auto *h265Profile = h265->add_profiles();
        h265Profile->set_type(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::H265_PROFILE_MAIN_10);
        h265Profile->set_max_level(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::H265_LEVEL_5_1);
        h265Profile->set_max_bitrate_in_bps(50000000);
        h265->add_dynamic_ranges(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::DYNAMIC_RANGE_HDR10);
        auto *vp9 = cap->mutable_codec_capabilities()->mutable_vp9();
        auto *vp9Profile = vp9->add_profiles();
        vp9Profile->set_type(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::VP9_PROFILE_0);
        vp9Profile->set_max_level(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::VP9_LEVEL_4);
        vp9Profile->set_max_bitrate_in_bps(30000000);
        vp9->add_dynamic_ranges(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::DYNAMIC_RANGE_SDR);
        auto *av1 = cap->mutable_codec_capabilities()->mutable_av1();
        auto *av1Profile = av1->add_profiles();
        av1Profile->set_type(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::AV1_PROFILE_MAIN);
        av1Profile->set_max_level(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::AV1_LEVEL_5_1);
        av1Profile->set_max_bitrate_in_bps(20000000);
        av1->add_dynamic_ranges(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::DYNAMIC_RANGE_SDR);
        auto *mpeg2 = cap->mutable_codec_capabilities()->mutable_mpeg2();
        auto *mpeg2Profile = mpeg2->add_profiles();
        mpeg2Profile->set_type(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::MPEG2_PROFILE_MAIN);
        mpeg2Profile->set_max_level(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::MPEG2_LEVEL_MAIN);
        mpeg2Profile->set_max_bitrate_in_bps(15000000);
        mpeg2->add_dynamic_ranges(firebolt::rialto::GetSupportedVideoCapabilitiesResponse::DYNAMIC_RANGE_SDR);
    }
};

TEST_F(MediaPipelineCapabilitiesIpcTest, createMediaPipelineCapabilitiesIpc)
{
    createMediaPipelineCapabilitiesIpc();
}

TEST_F(MediaPipelineCapabilitiesIpcTest, createMediaPipelineCapabilitiesTestAttachChannelFailure)
{
    expectInitIpcButAttachChannelFailure();
    EXPECT_THROW(m_sut = std::make_unique<MediaPipelineCapabilitiesIpc>(*m_ipcClientMock), std::runtime_error);
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedMimeTypesSuccess)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedMimeTypes"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setGetSupportedMimeTypesResponse)));

    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), m_mimeTypes);
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedMimeTypesDisconnected)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallDisconnected();

    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), std::vector<std::string>{});
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedMimeTypesDisconnectedReconnectChannel)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedMimeTypes"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setGetSupportedMimeTypesResponse)));

    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), m_mimeTypes);
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedMimeTypesFailure)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedMimeTypes"), _, _, _, _));

    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), std::vector<std::string>{});
}

TEST_F(MediaPipelineCapabilitiesIpcTest, IsMimeTypeSupportedSuccess)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isMimeTypeSupported"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setIsMimeTypeSupportedResponse)));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedPropertiesDisconnectedReconnectChannel)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedProperties"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setGetSupportedPropertiesResponse)));

    std::vector<std::string> propertiesToLookFor{kPropertyName};
    std::vector<std::string> propertiesSupported{
        m_sut->getSupportedProperties(MediaSourceType::VIDEO, propertiesToLookFor)};
    EXPECT_EQ(propertiesToLookFor, propertiesSupported);
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedPropertiesFailure)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedProperties"), _, _, _, _));

    std::vector<std::string> propertiesToLookFor{kPropertyName};
    std::vector<std::string> propertiesSupported{
        m_sut->getSupportedProperties(MediaSourceType::VIDEO, propertiesToLookFor)};
    EXPECT_TRUE(propertiesSupported.empty());
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedPropertiesSuccess)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedProperties"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setGetSupportedPropertiesResponse)));

    std::vector<std::string> propertiesToLookFor{kPropertyName};
    std::vector<std::string> propertiesSupported{
        m_sut->getSupportedProperties(MediaSourceType::VIDEO, propertiesToLookFor)};
    EXPECT_EQ(propertiesToLookFor, propertiesSupported);
}

TEST_F(MediaPipelineCapabilitiesIpcTest, IsMimeTypeSupportedsDisconnected)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallDisconnected();

    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, IsMimeTypeSupportedDisconnectedReconnectChannel)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isMimeTypeSupported"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setIsMimeTypeSupportedResponse)));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, IsMimeTypeSupportedFailure)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isMimeTypeSupported"), _, _, _, _));

    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedSubtitlesMimeTypesSuccess)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedMimeTypes"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setGetSupportedMimeTypesResponse)));

    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::SUBTITLE), m_mimeTypes);
}

TEST_F(MediaPipelineCapabilitiesIpcTest, IsVideoMasterSuccess)
{
    bool isMaster{false};

    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("isVideoMaster"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setIsVideoMasterResponse)));

    EXPECT_TRUE(m_sut->isVideoMaster(isMaster));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, IsVideoMastersDisconnected)
{
    bool isMaster{false};

    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallDisconnected();

    EXPECT_FALSE(m_sut->isVideoMaster(isMaster));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, IsVideoMasterDisconnectedReconnectChannel)
{
    bool isMaster{false};

    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("isVideoMaster"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setIsVideoMasterResponse)));

    EXPECT_TRUE(m_sut->isVideoMaster(isMaster));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, IsVideoMasterFailure)
{
    bool isMaster{false};

    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isVideoMaster"), _, _, _, _));

    EXPECT_FALSE(m_sut->isVideoMaster(isMaster));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedAudioCapabilitiesSuccess)
{
    AudioDecoderCapability audioCap;
    audioCap.pcm = PcmCapability{{1536000, 8, 192000, 32}};
    audioCap.aac = AacCapability{{{AacProfile::LC, AudioProfileCapability{576000, 8, 96000, 24}}}};
    audioCap.mp3 = Mp3Capability{{320000, 2, 48000, 16}};
    audioCap.alac = AlacCapability{{1000000, 8, 96000, 32}};
    audioCap.sbc = SbcCapability{{320000, 2, 44100, 16}};
    audioCap.dolbyAc3 = DolbyAc3Capability{{{DolbyAc3Profile::STANDARD, AudioProfileCapability{640000, 6, 48000, 24}}}};
    audioCap.dolbyAc4 = DolbyAc4Capability{{2688000, 16, 48000, 24}};
    audioCap.dolbyEac3 = DolbyEac3Capability{{{DolbyEac3Profile::PLUS, AudioProfileCapability{6144000, 8, 48000, 24}}}};
    audioCap.dolbyTruehd = DolbyTruehdCapability{{18000000, 8, 192000, 24}};
    audioCap.flac = FlacCapability{{1000000, 8, 192000, 32}};
    audioCap.vorbis = VorbisCapability{{500000, 8, 48000, 16}};
    audioCap.opus = OpusCapability{{510000, 8, 48000, 16}};
    const AudioDecoderCapabilities kCapabilities{kInterfaceVersion, kSchemaVersion, {audioCap}};

    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedAudioCapabilities"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setGetSupportedAudioCapabilitiesResponse)));

    EXPECT_THAT(m_sut->getSupportedAudioCapabilities(), decoderCapabilitiesMatcher(kCapabilities));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedAudioCapabilitiesDisconnected)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallDisconnected();

    m_sut->getSupportedAudioCapabilities();
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedAudioCapabilitiesReconnectChannel)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedAudioCapabilities"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setGetSupportedAudioCapabilitiesResponse)));

    m_sut->getSupportedAudioCapabilities();
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedAudioCapabilitiesFailure)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedAudioCapabilities"), _, _, _, _));

    m_sut->getSupportedAudioCapabilities();
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedVideoCapabilitiesSuccess)
{
    H264Profile h264Prof{H264ProfileType::H264_HIGH, H264Level::H264_LEVEL_5_1, 50000000};
    H264CodecCapability h264Codec{{{h264Prof}}, {DynamicRange::SDR, DynamicRange::HDR10}};
    H265Profile h265Prof{H265ProfileType::H265_MAIN_10, H265Level::H265_LEVEL_5_1, 50000000};
    H265CodecCapability h265Codec{{{h265Prof}}, {DynamicRange::HDR10}};
    Vp9Profile vp9Prof{Vp9ProfileType::VP9_PROFILE_0, Vp9Level::VP9_LEVEL_4, 30000000};
    Vp9CodecCapability vp9Codec{{{vp9Prof}}, {DynamicRange::SDR}};
    Av1Profile av1Prof{Av1ProfileType::AV1_MAIN, Av1Level::AV1_LEVEL_5_1, 20000000};
    Av1CodecCapability av1Codec{{{av1Prof}}, {DynamicRange::SDR}};
    Mpeg2Profile mpeg2Prof{Mpeg2ProfileType::MPEG2_MAIN, Mpeg2Level::MPEG2_LEVEL_MAIN, 15000000};
    Mpeg2CodecCapability mpeg2Codec{{{mpeg2Prof}}, {DynamicRange::SDR}};
    VideoCodecCapabilities codecs;
    codecs.h264 = std::move(h264Codec);
    codecs.h265 = std::move(h265Codec);
    codecs.vp9 = std::move(vp9Codec);
    codecs.av1 = std::move(av1Codec);
    codecs.mpeg2 = std::move(mpeg2Codec);
    VideoDecoderCapability videoCap;
    videoCap.codecCapabilities = std::move(codecs);
    const VideoDecoderCapabilities kCapabilities{kInterfaceVersion, kSchemaVersion, {videoCap}};

    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedVideoCapabilities"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setGetSupportedVideoCapabilitiesResponse)));

    EXPECT_THAT(m_sut->getSupportedVideoCapabilities(), decoderCapabilitiesMatcher(kCapabilities));
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedVideoCapabilitiesDisconnected)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallDisconnected();

    m_sut->getSupportedVideoCapabilities();
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedVideoCapabilitiesReconnectChannel)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedVideoCapabilities"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesIpcTest::setGetSupportedVideoCapabilitiesResponse)));

    m_sut->getSupportedVideoCapabilities();
}

TEST_F(MediaPipelineCapabilitiesIpcTest, GetSupportedVideoCapabilitiesFailure)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedVideoCapabilities"), _, _, _, _));

    m_sut->getSupportedVideoCapabilities();
}
