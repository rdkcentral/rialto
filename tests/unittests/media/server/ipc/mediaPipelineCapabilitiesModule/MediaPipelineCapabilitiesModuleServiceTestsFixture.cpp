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

#include "MediaPipelineCapabilitiesModuleServiceTestsFixture.h"
#include "AudioDecoderCapabilities.h"
#include "MediaCommon.h"
#include "MediaPipelineCapabilitiesModuleService.h"
#include "RialtoCommonModule.h"
#include "VideoDecoderCapabilities.h"

#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SetArgReferee;

namespace
{
const std::vector<std::string> kMimeTypes{"video/h264", "video/h265"};
const firebolt::rialto::MediaSourceType kSourceType{firebolt::rialto::MediaSourceType::VIDEO};
const firebolt::rialto::ProtoMediaSourceType kMediaSourceType{firebolt::rialto::ProtoMediaSourceType::VIDEO};
const std::vector<std::string> kPropertyNames{"test-property", "another-property"};
constexpr bool kIsVideoMaster{true};
const firebolt::rialto::common::AudioDecoderCapabilities kAudioCapabilities = []()
{
    using namespace firebolt::rialto::common;
    AudioDecoderCapability cap;
    cap.pcm = PcmCapability{{1536000, 8, 192000, 32}};
    cap.aac = AacCapability{{{AacProfile::LC, AudioProfileCapability{576000, 8, 96000, 24}}}};
    cap.mp3 = Mp3Capability{{320000, 2, 48000, 16}};
    cap.alac = AlacCapability{{1000000, 8, 96000, 32}};
    cap.sbc = SbcCapability{{320000, 2, 44100, 16}};
    cap.dolbyAc3 = DolbyAc3Capability{{{DolbyAc3Profile::STANDARD, AudioProfileCapability{640000, 6, 48000, 24}}}};
    cap.dolbyAc4 = DolbyAc4Capability{{2688000, 16, 48000, 24}};
    cap.dolbyEac3 = DolbyEac3Capability{{{DolbyEac3Profile::PLUS, AudioProfileCapability{6144000, 8, 48000, 24}}}};
    cap.dolbyTruehd = DolbyTruehdCapability{{18000000, 8, 192000, 24}};
    cap.flac = FlacCapability{{1000000, 8, 192000, 32}};
    cap.vorbis = VorbisCapability{{500000, 8, 48000, 16}};
    cap.opus = OpusCapability{{510000, 8, 48000, 16}};
    cap.mpegAudio = MpegAudioCapability{{{MpegAudioProfile::LAYER_2, AudioProfileCapability{384000, 2, 48000, 16}}}};
    cap.realAudio = RealAudioCapability{{{RealAudioProfile::RA8, AudioProfileCapability{128000, 2, 44100, 16}}}};
    cap.usac = UsacCapability{{{UsacProfile::BASELINE, AudioProfileCapability{256000, 2, 48000, 24}}}};
    cap.dts = DtsCapability{{{DtsProfile::CORE, AudioProfileCapability{1536000, 6, 48000, 24}}}};
    cap.avs = AvsCapability{{{AvsProfile::AVS2, AudioProfileCapability{512000, 8, 48000, 24}}}};
    return AudioDecoderCapabilities{"1.0", "2.0", {cap}};
}();
const firebolt::rialto::common::VideoDecoderCapabilities kVideoCapabilities = []()
{
    using namespace firebolt::rialto::common;
    VideoDecoderCapability videoCap;
    videoCap.codecCapabilities.h264 =
        H264CodecCapability{{{H264ProfileType::H264_HIGH, H264Level::H264_LEVEL_5_1, 50000000u},
                             {H264ProfileType::H264_BASELINE, H264Level::H264_LEVEL_3, 8000000u},
                             {H264ProfileType::H264_MAIN, H264Level::H264_LEVEL_3_1, 14000000u}},
                            {DynamicRange::SDR, DynamicRange::HDR10, DynamicRange::HLG, DynamicRange::HDR10PLUS,
                             DynamicRange::DOLBY_VISION}};
    videoCap.codecCapabilities.h265 =
        H265CodecCapability{{{H265ProfileType::H265_MAIN_10, H265Level::H265_LEVEL_5_1, 50000000u},
                             {H265ProfileType::H265_MAIN, H265Level::H265_LEVEL_4, 10000000u},
                             {H265ProfileType::H265_MAIN_10_HDR10, H265Level::H265_LEVEL_6, 80000000u}},
                            {DynamicRange::HDR10}};
    videoCap.codecCapabilities.vp9 =
        Vp9CodecCapability{{{Vp9ProfileType::VP9_PROFILE_0, Vp9Level::VP9_LEVEL_4, 30000000u},
                            {Vp9ProfileType::VP9_PROFILE_1, Vp9Level::VP9_LEVEL_1, 200000u},
                            {Vp9ProfileType::VP9_PROFILE_2, Vp9Level::VP9_LEVEL_1_1, 400000u},
                            {Vp9ProfileType::VP9_PROFILE_3, Vp9Level::VP9_LEVEL_2, 1500000u}},
                           {DynamicRange::SDR}};
    videoCap.codecCapabilities.av1 = Av1CodecCapability{{{Av1ProfileType::AV1_MAIN, Av1Level::AV1_LEVEL_5_1, 20000000u},
                                                         {Av1ProfileType::AV1_HIGH, Av1Level::AV1_LEVEL_4_0, 6000000u}},
                                                        {DynamicRange::SDR}};
    videoCap.codecCapabilities.mpeg2 =
        Mpeg2CodecCapability{{{Mpeg2ProfileType::MPEG2_MAIN, Mpeg2Level::MPEG2_LEVEL_MAIN, 15000000u},
                              {Mpeg2ProfileType::MPEG2_SIMPLE, Mpeg2Level::MPEG2_LEVEL_LOW, 4000000u},
                              {Mpeg2ProfileType::MPEG2_MAIN, Mpeg2Level::MPEG2_LEVEL_HIGH, 80000000u}},
                             {DynamicRange::SDR}};
    return VideoDecoderCapabilities{"3.0", "4.0", {videoCap}};
}();
} // namespace

MediaPipelineCapabilitiesModuleServiceTests::MediaPipelineCapabilitiesModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()},
      m_invalidControllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::RpcControllerMock>>()},
      m_mediaCapabilitiesMock{std::make_shared<StrictMock<firebolt::rialto::IMediaCapabilitiesMock>>()}
{
    m_service = std::make_shared<firebolt::rialto::server::ipc::MediaPipelineCapabilitiesModuleService>(
        m_mediaPipelineServiceMock);
}

MediaPipelineCapabilitiesModuleServiceTests::~MediaPipelineCapabilitiesModuleServiceTests() {}

void MediaPipelineCapabilitiesModuleServiceTests::clientWillConnect()
{
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineServiceWillGetSupportedMimeTypes()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, getSupportedMimeTypes(kSourceType)).WillOnce(Return(kMimeTypes));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineWillCheckIfMimeTypeIsSupported()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, isMimeTypeSupported(kMimeTypes[0])).WillOnce(Return(true));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineWillGetSupportedProperties()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock,
                getSupportedProperties(firebolt::rialto::server::ipc::convertMediaSourceType(kMediaSourceType),
                                       kPropertyNames))
        .WillOnce(Return(kPropertyNames));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineWillCheckIfVideoIsMaster()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, isVideoMaster(_))
        .WillOnce(DoAll(SetArgReferee<0>(kIsVideoMaster), Return(true)));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineWillGetSupportedAudioCapabilities()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, getSupportedAudioCapabilities()).WillOnce(Return(kAudioCapabilities));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineWillGetSupportedVideoCapabilities()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, getSupportedVideoCapabilities()).WillOnce(Return(kVideoCapabilities));
}

void MediaPipelineCapabilitiesModuleServiceTests::expectRequestSuccess()
{
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaPipelineCapabilitiesModuleServiceTests::expectInvalidControlFailure()
{
    EXPECT_CALL(*m_invalidControllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaPipelineCapabilitiesModuleServiceTests::sendClientConnected()
{
    m_service->clientConnected(m_clientMock);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedMimeTypesRequestAndReceiveResponse()
{
    firebolt::rialto::GetSupportedMimeTypesRequest request;
    firebolt::rialto::GetSupportedMimeTypesResponse response;

    request.set_media_type(firebolt::rialto::ProtoMediaSourceType::VIDEO);

    m_service->getSupportedMimeTypes(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ((std::vector<std::string>{response.mime_types().begin(), response.mime_types().end()}), kMimeTypes);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedMimeTypesRequestAndExpectFailure()
{
    firebolt::rialto::GetSupportedMimeTypesRequest request;
    firebolt::rialto::GetSupportedMimeTypesResponse response;

    m_service->getSupportedMimeTypes(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_TRUE(response.mime_types().empty());
}

void MediaPipelineCapabilitiesModuleServiceTests::sendIsMimeTypeSupportedRequestAndReceiveResponse()
{
    firebolt::rialto::IsMimeTypeSupportedRequest request;
    firebolt::rialto::IsMimeTypeSupportedResponse response;

    request.set_mime_type(kMimeTypes[0]);

    m_service->isMimeTypeSupported(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.is_supported(), true);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendIsMimeTypeSupportedRequestAndExpectFailure()
{
    firebolt::rialto::IsMimeTypeSupportedRequest request;
    firebolt::rialto::IsMimeTypeSupportedResponse response;

    m_service->isMimeTypeSupported(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.is_supported(), false);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedPropertiesRequestWithSuccess()
{
    firebolt::rialto::GetSupportedPropertiesRequest request;
    firebolt::rialto::GetSupportedPropertiesResponse response;

    request.set_media_type(kMediaSourceType);
    for (const std::string &property : kPropertyNames)
        request.add_property_names(property.c_str());

    m_service->getSupportedProperties(m_controllerMock.get(), &request, &response, m_closureMock.get());

    std::vector<std::string> supportedProperties{response.supported_properties().begin(),
                                                 response.supported_properties().end()};
    EXPECT_EQ(supportedProperties, kPropertyNames);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedPropertiesRequestAndExpectFailure()
{
    firebolt::rialto::GetSupportedPropertiesRequest request;
    firebolt::rialto::GetSupportedPropertiesResponse response;

    request.set_media_type(kMediaSourceType);
    for (const std::string &property : kPropertyNames)
        request.add_property_names(property.c_str());

    m_service->getSupportedProperties(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());

    std::vector<std::string> supportedProperties{response.supported_properties().begin(),
                                                 response.supported_properties().end()};
    EXPECT_TRUE(supportedProperties.empty());
}

void MediaPipelineCapabilitiesModuleServiceTests::sendIsVideoMasterRequestWithSuccess()
{
    firebolt::rialto::IsVideoMasterRequest request;
    firebolt::rialto::IsVideoMasterResponse response;

    m_service->isVideoMaster(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.is_video_master(), kIsVideoMaster);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendIsVideoMasterRequestAndExpectFailure()
{
    firebolt::rialto::IsVideoMasterRequest request;
    firebolt::rialto::IsVideoMasterResponse response;
    m_service->isVideoMaster(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedAudioCapabilitiesRequestAndReceiveResponse()
{
    firebolt::rialto::GetSupportedAudioCapabilitiesRequest request;
    firebolt::rialto::GetSupportedAudioCapabilitiesResponse response;
    m_service->getSupportedAudioCapabilities(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.interface_version(), kAudioCapabilities.interfaceVersion);
    EXPECT_EQ(response.schema_version(), kAudioCapabilities.schemaVersion);
    ASSERT_EQ(response.capabilities_size(), 1);
    const auto &cap = response.capabilities(0);
    ASSERT_TRUE(cap.has_pcm());
    EXPECT_EQ(cap.pcm().base().max_bitrate_in_bps(), 1536000u);
    ASSERT_TRUE(cap.has_aac());
    ASSERT_EQ(cap.aac().profiles_size(), 1);
    ASSERT_TRUE(cap.has_mp3());
    ASSERT_TRUE(cap.has_alac());
    ASSERT_TRUE(cap.has_sbc());
    ASSERT_TRUE(cap.has_dolby_ac3());
    ASSERT_TRUE(cap.has_dolby_ac4());
    ASSERT_TRUE(cap.has_dolby_eac3());
    ASSERT_TRUE(cap.has_dolby_truehd());
    ASSERT_TRUE(cap.has_flac());
    ASSERT_TRUE(cap.has_vorbis());
    ASSERT_TRUE(cap.has_opus());
    ASSERT_TRUE(cap.has_mpeg_audio());
    ASSERT_TRUE(cap.has_real_audio());
    ASSERT_TRUE(cap.has_usac());
    ASSERT_TRUE(cap.has_dts());
    ASSERT_TRUE(cap.has_avs());
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedAudioCapabilitiesRequestAndExpectFailure()
{
    firebolt::rialto::GetSupportedAudioCapabilitiesRequest request;
    firebolt::rialto::GetSupportedAudioCapabilitiesResponse response;
    m_service->getSupportedAudioCapabilities(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedVideoCapabilitiesRequestAndReceiveResponse()
{
    firebolt::rialto::GetSupportedVideoCapabilitiesRequest request;
    firebolt::rialto::GetSupportedVideoCapabilitiesResponse response;
    m_service->getSupportedVideoCapabilities(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.interface_version(), kVideoCapabilities.interfaceVersion);
    EXPECT_EQ(response.schema_version(), kVideoCapabilities.schemaVersion);
    ASSERT_EQ(response.capabilities_size(), 1);
    const auto &codecs = response.capabilities(0).codec_capabilities();
    ASSERT_TRUE(codecs.has_h264());
    ASSERT_EQ(codecs.h264().profiles_size(), 3);
    EXPECT_EQ(codecs.h264().profiles(0).type(),
              firebolt::rialto::GetSupportedVideoCapabilitiesResponse::H264_PROFILE_HIGH);
    EXPECT_EQ(codecs.h264().profiles(0).max_level(),
              firebolt::rialto::GetSupportedVideoCapabilitiesResponse::H264_LEVEL_5_1);
    EXPECT_EQ(codecs.h264().profiles(0).max_bitrate_in_bps(), 50000000u);
    ASSERT_EQ(codecs.h264().dynamic_ranges_size(), 5);
    ASSERT_TRUE(codecs.has_h265());
    ASSERT_EQ(codecs.h265().profiles_size(), 3);
    ASSERT_TRUE(codecs.has_vp9());
    ASSERT_EQ(codecs.vp9().profiles_size(), 4);
    ASSERT_TRUE(codecs.has_av1());
    ASSERT_EQ(codecs.av1().profiles_size(), 2);
    ASSERT_TRUE(codecs.has_mpeg2());
    ASSERT_EQ(codecs.mpeg2().profiles_size(), 3);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedVideoCapabilitiesRequestAndExpectFailure()
{
    firebolt::rialto::GetSupportedVideoCapabilitiesRequest request;
    firebolt::rialto::GetSupportedVideoCapabilitiesResponse response;
    m_service->getSupportedVideoCapabilities(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineCapabilitiesModuleServiceTests::expectCorrectMediaTypeConversion()
{
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(firebolt::rialto::ProtoMediaSourceType::UNKNOWN),
              firebolt::rialto::MediaSourceType::UNKNOWN);
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(firebolt::rialto::ProtoMediaSourceType::AUDIO),
              firebolt::rialto::MediaSourceType::AUDIO);
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(firebolt::rialto::ProtoMediaSourceType::VIDEO),
              firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(firebolt::rialto::ProtoMediaSourceType::SUBTITLE),
              firebolt::rialto::MediaSourceType::SUBTITLE);
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(
                  static_cast<firebolt::rialto::ProtoMediaSourceType>(4)),
              firebolt::rialto::MediaSourceType::UNKNOWN);
}
