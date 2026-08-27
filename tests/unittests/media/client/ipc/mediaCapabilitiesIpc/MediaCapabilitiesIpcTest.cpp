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

#include "MediaCapabilitiesIpc.h"
#include "IpcModuleBase.h"
#include <gtest/gtest.h>

using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArgs;

namespace
{
const std::string kInterfaceVersion{"2.0.0"};
const std::string kSchemaVersion{"1.0.0"};
} // namespace

class MediaCapabilitiesIpcTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<firebolt::rialto::IMediaCapabilities> m_sut;

    void createMediaCapabilitiesIpc()
    {
        expectInitIpc();
        EXPECT_NO_THROW(m_sut = std::make_unique<firebolt::rialto::client::MediaCapabilitiesIpc>(*m_ipcClientMock));
    }

public:
    void setGetSupportedAudioCapabilitiesResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSupportedAudioCapabilitiesResponse *getSupportedAudioCapabResp =
            dynamic_cast<firebolt::rialto::GetSupportedAudioCapabilitiesResponse *>(response);
        getSupportedAudioCapabResp->set_interface_version(kInterfaceVersion);
        getSupportedAudioCapabResp->set_schema_version(kSchemaVersion);

        // Add a single audio capability entry with various codec formats
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
    }

    void setGetSupportedVideoCapabilitiesResponse(google::protobuf::Message *response)
    {
        using R = firebolt::rialto::GetSupportedVideoCapabilitiesResponse;
        auto *getSupportedVideoCapabResp = dynamic_cast<R *>(response);
        getSupportedVideoCapabResp->set_interface_version(kInterfaceVersion);
        getSupportedVideoCapabResp->set_schema_version(kSchemaVersion);

        auto *cap = getSupportedVideoCapabResp->add_capabilities();

        // H264 codec capabilities
        auto *h264 = cap->mutable_codec_capabilities()->mutable_h264();
        auto *h264Profile = h264->add_profiles();
        h264Profile->set_type(R::H264_PROFILE_HIGH);
        h264Profile->set_max_level(R::H264_LEVEL_5_1);
        h264Profile->set_max_bitrate_in_bps(50000000);
        h264->add_dynamic_ranges(R::DYNAMIC_RANGE_SDR);
        h264->add_dynamic_ranges(R::DYNAMIC_RANGE_HDR10);

        // H265 codec capabilities
        auto *h265 = cap->mutable_codec_capabilities()->mutable_h265();
        auto *h265Profile = h265->add_profiles();
        h265Profile->set_type(R::H265_PROFILE_MAIN);
        h265Profile->set_max_level(R::H265_LEVEL_5);
        h265Profile->set_max_bitrate_in_bps(25000000);
        h265->add_dynamic_ranges(R::DYNAMIC_RANGE_HDR10);
    }
};

TEST_F(MediaCapabilitiesIpcTest, createMediaCapabilitiesIpc)
{
    createMediaCapabilitiesIpc();
    EXPECT_NE(m_sut, nullptr);
}

TEST_F(MediaCapabilitiesIpcTest, createMediaCapabilitiesIpcAttachChannelFailure)
{
    expectInitIpcButAttachChannelFailure();
    EXPECT_THROW(m_sut = std::make_unique<firebolt::rialto::client::MediaCapabilitiesIpc>(*m_ipcClientMock),
                 std::runtime_error);
}

TEST_F(MediaCapabilitiesIpcTest, GetSupportedAudioCapabilitiesSuccess)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedAudioCapabilities"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaCapabilitiesIpcTest::setGetSupportedAudioCapabilitiesResponse)));

    firebolt::rialto::common::AudioDecoderCapabilities result = m_sut->getSupportedAudioCapabilities();
    EXPECT_EQ(result.interfaceVersion, kInterfaceVersion);
    EXPECT_EQ(result.schemaVersion, kSchemaVersion);
    EXPECT_GT(result.capabilities.size(), 0);
}

TEST_F(MediaCapabilitiesIpcTest, GetSupportedAudioCapabilitiesDisconnected)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallDisconnected();

    firebolt::rialto::common::AudioDecoderCapabilities result = m_sut->getSupportedAudioCapabilities();
    // On disconnect, empty capabilities are returned
    EXPECT_TRUE(result.interfaceVersion.empty());
    EXPECT_TRUE(result.schemaVersion.empty());
}

TEST_F(MediaCapabilitiesIpcTest, GetSupportedAudioCapabilitiesDisconnectedReconnectChannel)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedAudioCapabilities"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaCapabilitiesIpcTest::setGetSupportedAudioCapabilitiesResponse)));

    firebolt::rialto::common::AudioDecoderCapabilities result = m_sut->getSupportedAudioCapabilities();
    EXPECT_EQ(result.interfaceVersion, kInterfaceVersion);
    EXPECT_EQ(result.schemaVersion, kSchemaVersion);
}

TEST_F(MediaCapabilitiesIpcTest, GetSupportedAudioCapabilitiesFailure)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedAudioCapabilities"), _, _, _, _));

    firebolt::rialto::common::AudioDecoderCapabilities result = m_sut->getSupportedAudioCapabilities();
    // On RPC failure, empty capabilities are returned
    EXPECT_TRUE(result.interfaceVersion.empty());
    EXPECT_TRUE(result.schemaVersion.empty());
}

TEST_F(MediaCapabilitiesIpcTest, GetSupportedVideoCapabilitiesSuccess)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedVideoCapabilities"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaCapabilitiesIpcTest::setGetSupportedVideoCapabilitiesResponse)));

    firebolt::rialto::common::VideoDecoderCapabilities result = m_sut->getSupportedVideoCapabilities();
    EXPECT_EQ(result.interfaceVersion, kInterfaceVersion);
    EXPECT_EQ(result.schemaVersion, kSchemaVersion);
    EXPECT_GT(result.capabilities.size(), 0);
}

TEST_F(MediaCapabilitiesIpcTest, GetSupportedVideoCapabilitiesDisconnected)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallDisconnected();

    firebolt::rialto::common::VideoDecoderCapabilities result = m_sut->getSupportedVideoCapabilities();
    // On disconnect, empty capabilities are returned
    EXPECT_TRUE(result.interfaceVersion.empty());
    EXPECT_TRUE(result.schemaVersion.empty());
}

TEST_F(MediaCapabilitiesIpcTest, GetSupportedVideoCapabilitiesDisconnectedReconnectChannel)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedVideoCapabilities"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaCapabilitiesIpcTest::setGetSupportedVideoCapabilitiesResponse)));

    firebolt::rialto::common::VideoDecoderCapabilities result = m_sut->getSupportedVideoCapabilities();
    EXPECT_EQ(result.interfaceVersion, kInterfaceVersion);
    EXPECT_EQ(result.schemaVersion, kSchemaVersion);
}

TEST_F(MediaCapabilitiesIpcTest, GetSupportedVideoCapabilitiesFailure)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedVideoCapabilities"), _, _, _, _));

    firebolt::rialto::common::VideoDecoderCapabilities result = m_sut->getSupportedVideoCapabilities();
    // On RPC failure, empty capabilities are returned
    EXPECT_TRUE(result.interfaceVersion.empty());
    EXPECT_TRUE(result.schemaVersion.empty());
}
