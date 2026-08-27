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

#include <gtest/gtest.h>

#include "IpcModuleBase.h"
#include "MediaCapabilitiesIpc.h"

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::Invoke;
using ::testing::WithArgs;

class MediaCapabilitiesTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<firebolt::rialto::IMediaCapabilities> m_sut;

    void createMediaCapabilitiesIpc()
    {
        expectInitIpc();
        EXPECT_NO_THROW(m_sut = std::make_unique<firebolt::rialto::client::MediaCapabilitiesIpc>(*m_ipcClientMock));
    }

public:
    void setAudioResponse(google::protobuf::Message *message)
    {
        auto *response = dynamic_cast<GetSupportedAudioCapabilitiesResponse *>(message);
        ASSERT_NE(response, nullptr);
        response->set_interface_version("1.0");
        response->set_schema_version("2.0");
        response->add_capabilities()->mutable_pcm()->mutable_base()->set_max_channels(2);
    }

    void setVideoResponse(google::protobuf::Message *message)
    {
        auto *response = dynamic_cast<GetSupportedVideoCapabilitiesResponse *>(message);
        ASSERT_NE(response, nullptr);
        response->set_interface_version("1.0");
        response->set_schema_version("2.0");
        auto *profile = response->add_capabilities()->mutable_codec_capabilities()->mutable_h264()->add_profiles();
        profile->set_type(GetSupportedVideoCapabilitiesResponse::H264_PROFILE_HIGH);
        profile->set_max_level(GetSupportedVideoCapabilitiesResponse::H264_LEVEL_4_1);
    }
};

TEST_F(MediaCapabilitiesTest, AudioDecoderCapabilities)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedAudioCapabilities"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaCapabilitiesTest::setAudioResponse)));

    const auto result = m_sut->getSupportedAudioCapabilities();
    ASSERT_EQ(result.capabilities.size(), 1U);
    ASSERT_TRUE(result.capabilities.front().pcm.has_value());
    EXPECT_EQ(result.capabilities.front().pcm->base.maxChannels, 2U);
}

TEST_F(MediaCapabilitiesTest, VideoDecoderCapabilities)
{
    createMediaCapabilitiesIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedVideoCapabilities"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaCapabilitiesTest::setVideoResponse)));

    const auto result = m_sut->getSupportedVideoCapabilities();
    ASSERT_EQ(result.capabilities.size(), 1U);
    ASSERT_TRUE(result.capabilities.front().codecCapabilities.h264.has_value());
    ASSERT_EQ(result.capabilities.front().codecCapabilities.h264->profiles.size(), 1U);
    EXPECT_EQ(result.capabilities.front().codecCapabilities.h264->profiles.front().type,
              common::H264ProfileType::H264_HIGH);
}
