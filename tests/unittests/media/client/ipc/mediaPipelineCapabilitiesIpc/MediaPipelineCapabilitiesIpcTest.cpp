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
#include "MediaCapabilitiesIpcConverters.h"
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
        auto *mpegAudioEntry = cap->mutable_mpeg_audio()->add_profiles();
        mpegAudioEntry->set_profile(firebolt::rialto::GetSupportedAudioCapabilitiesResponse::MPEG_AUDIO_PROFILE_LAYER_2);
        mpegAudioEntry->mutable_capability()->set_max_bitrate_in_bps(384000);
        mpegAudioEntry->mutable_capability()->set_max_channels(2);
        mpegAudioEntry->mutable_capability()->set_max_sample_rate_in_hz(48000);
        mpegAudioEntry->mutable_capability()->set_max_bit_depth(16);
        auto *realAudioEntry = cap->mutable_real_audio()->add_profiles();
        realAudioEntry->set_profile(firebolt::rialto::GetSupportedAudioCapabilitiesResponse::REALAUDIO_PROFILE_RA8);
        realAudioEntry->mutable_capability()->set_max_bitrate_in_bps(128000);
        realAudioEntry->mutable_capability()->set_max_channels(2);
        realAudioEntry->mutable_capability()->set_max_sample_rate_in_hz(44100);
        realAudioEntry->mutable_capability()->set_max_bit_depth(16);
        auto *usacEntry = cap->mutable_usac()->add_profiles();
        usacEntry->set_profile(firebolt::rialto::GetSupportedAudioCapabilitiesResponse::USAC_PROFILE_BASELINE);
        usacEntry->mutable_capability()->set_max_bitrate_in_bps(256000);
        usacEntry->mutable_capability()->set_max_channels(2);
        usacEntry->mutable_capability()->set_max_sample_rate_in_hz(48000);
        usacEntry->mutable_capability()->set_max_bit_depth(24);
        auto *dtsEntry = cap->mutable_dts()->add_profiles();
        dtsEntry->set_profile(firebolt::rialto::GetSupportedAudioCapabilitiesResponse::DTS_PROFILE_CORE);
        dtsEntry->mutable_capability()->set_max_bitrate_in_bps(1536000);
        dtsEntry->mutable_capability()->set_max_channels(6);
        dtsEntry->mutable_capability()->set_max_sample_rate_in_hz(48000);
        dtsEntry->mutable_capability()->set_max_bit_depth(24);
        auto *avsEntry = cap->mutable_avs()->add_profiles();
        avsEntry->set_profile(firebolt::rialto::GetSupportedAudioCapabilitiesResponse::AVS_PROFILE_AVS2);
        avsEntry->mutable_capability()->set_max_bitrate_in_bps(512000);
        avsEntry->mutable_capability()->set_max_channels(8);
        avsEntry->mutable_capability()->set_max_sample_rate_in_hz(48000);
        avsEntry->mutable_capability()->set_max_bit_depth(24);
    }

    void setGetSupportedVideoCapabilitiesResponse(google::protobuf::Message *response)
    {
        using R = firebolt::rialto::GetSupportedVideoCapabilitiesResponse;
        auto *getSupportedVideoCapabResp = dynamic_cast<R *>(response);
        getSupportedVideoCapabResp->set_interface_version(kInterfaceVersion);
        getSupportedVideoCapabResp->set_schema_version(kSchemaVersion);
        auto *cap = getSupportedVideoCapabResp->add_capabilities();

        // H264: cover all profile types and all levels
        auto *h264 = cap->mutable_codec_capabilities()->mutable_h264();
        auto addH264 = [&](R::H264ProfileType t, R::H264Level l, uint32_t br)
        {
            auto *p = h264->add_profiles();
            p->set_type(t);
            p->set_max_level(l);
            p->set_max_bitrate_in_bps(br);
        };
        addH264(R::H264_PROFILE_HIGH, R::H264_LEVEL_5_1, 50000000);
        addH264(R::H264_PROFILE_BASELINE, R::H264_LEVEL_3, 8000000);
        addH264(R::H264_PROFILE_MAIN, R::H264_LEVEL_3_1, 14000000);
        addH264(R::H264_PROFILE_HIGH, R::H264_LEVEL_4, 20000000);
        addH264(R::H264_PROFILE_HIGH, R::H264_LEVEL_4_1, 25000000);
        addH264(R::H264_PROFILE_HIGH, R::H264_LEVEL_5, 40000000);
        addH264(R::H264_PROFILE_HIGH, R::H264_LEVEL_5_2, 60000000);
        h264->add_dynamic_ranges(R::DYNAMIC_RANGE_SDR);
        h264->add_dynamic_ranges(R::DYNAMIC_RANGE_HDR10);
        h264->add_dynamic_ranges(R::DYNAMIC_RANGE_HLG);
        h264->add_dynamic_ranges(R::DYNAMIC_RANGE_HDR10PLUS);
        h264->add_dynamic_ranges(R::DYNAMIC_RANGE_DOLBY_VISION);

        // H265: cover all profile types and all levels
        auto *h265 = cap->mutable_codec_capabilities()->mutable_h265();
        auto addH265 = [&](R::H265ProfileType t, R::H265Level l, uint32_t br)
        {
            auto *p = h265->add_profiles();
            p->set_type(t);
            p->set_max_level(l);
            p->set_max_bitrate_in_bps(br);
        };
        addH265(R::H265_PROFILE_MAIN_10, R::H265_LEVEL_5_1, 50000000);
        addH265(R::H265_PROFILE_MAIN, R::H265_LEVEL_4, 10000000);
        addH265(R::H265_PROFILE_MAIN, R::H265_LEVEL_4_1, 12000000);
        addH265(R::H265_PROFILE_MAIN, R::H265_LEVEL_5, 25000000);
        addH265(R::H265_PROFILE_MAIN_10_HDR10, R::H265_LEVEL_5_2, 35000000);
        addH265(R::H265_PROFILE_MAIN, R::H265_LEVEL_6, 60000000);
        addH265(R::H265_PROFILE_MAIN, R::H265_LEVEL_6_1, 80000000);
        addH265(R::H265_PROFILE_MAIN, R::H265_LEVEL_6_2, 100000000);
        h265->add_dynamic_ranges(R::DYNAMIC_RANGE_HDR10);

        // VP9: cover all profile types and all levels
        auto *vp9 = cap->mutable_codec_capabilities()->mutable_vp9();
        auto addVp9 = [&](R::Vp9ProfileType t, R::Vp9Level l, uint32_t br)
        {
            auto *p = vp9->add_profiles();
            p->set_type(t);
            p->set_max_level(l);
            p->set_max_bitrate_in_bps(br);
        };
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_4, 30000000);
        addVp9(R::VP9_PROFILE_1, R::VP9_LEVEL_1, 200000);
        addVp9(R::VP9_PROFILE_2, R::VP9_LEVEL_1_1, 400000);
        addVp9(R::VP9_PROFILE_3, R::VP9_LEVEL_2, 1500000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_2_1, 3000000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_3, 6000000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_3_1, 12000000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_4_1, 40000000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_5, 60000000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_5_1, 80000000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_5_2, 100000000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_6, 160000000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_6_1, 240000000);
        addVp9(R::VP9_PROFILE_0, R::VP9_LEVEL_6_2, 480000000);
        vp9->add_dynamic_ranges(R::DYNAMIC_RANGE_SDR);

        // AV1: cover all profile types and all levels
        auto *av1 = cap->mutable_codec_capabilities()->mutable_av1();
        auto addAv1 = [&](R::Av1ProfileType t, R::Av1Level l, uint32_t br)
        {
            auto *p = av1->add_profiles();
            p->set_type(t);
            p->set_max_level(l);
            p->set_max_bitrate_in_bps(br);
        };
        addAv1(R::AV1_PROFILE_MAIN, R::AV1_LEVEL_5_1, 20000000);
        addAv1(R::AV1_PROFILE_HIGH, R::AV1_LEVEL_4_0, 6000000);
        addAv1(R::AV1_PROFILE_MAIN, R::AV1_LEVEL_4_1, 8000000);
        addAv1(R::AV1_PROFILE_MAIN, R::AV1_LEVEL_5_0, 12000000);
        addAv1(R::AV1_PROFILE_MAIN, R::AV1_LEVEL_5_2, 25000000);
        addAv1(R::AV1_PROFILE_MAIN, R::AV1_LEVEL_6_0, 40000000);
        addAv1(R::AV1_PROFILE_MAIN, R::AV1_LEVEL_6_1, 60000000);
        addAv1(R::AV1_PROFILE_MAIN, R::AV1_LEVEL_6_2, 100000000);
        av1->add_dynamic_ranges(R::DYNAMIC_RANGE_SDR);

        // MPEG2: cover all profile types and all levels
        auto *mpeg2 = cap->mutable_codec_capabilities()->mutable_mpeg2();
        auto addMpeg2 = [&](R::Mpeg2ProfileType t, R::Mpeg2Level l, uint32_t br)
        {
            auto *p = mpeg2->add_profiles();
            p->set_type(t);
            p->set_max_level(l);
            p->set_max_bitrate_in_bps(br);
        };
        addMpeg2(R::MPEG2_PROFILE_MAIN, R::MPEG2_LEVEL_MAIN, 15000000);
        addMpeg2(R::MPEG2_PROFILE_SIMPLE, R::MPEG2_LEVEL_LOW, 4000000);
        addMpeg2(R::MPEG2_PROFILE_MAIN, R::MPEG2_LEVEL_HIGH, 80000000);
        mpeg2->add_dynamic_ranges(R::DYNAMIC_RANGE_SDR);
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
