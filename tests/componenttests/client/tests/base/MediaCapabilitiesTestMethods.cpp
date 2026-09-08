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

#include "MediaCapabilitiesTestMethods.h"
#include <utility>

using ::testing::_;
using ::testing::Invoke;

namespace firebolt::rialto::client::ct
{
MediaCapabilitiesTestMethods::MediaCapabilitiesTestMethods()
    : m_mediaCapabilitiesModuleMock{std::make_shared<::testing::StrictMock<MediaCapabilitiesModuleMock>>()}
{
}

MediaCapabilitiesTestMethods::~MediaCapabilitiesTestMethods() = default;

void MediaCapabilitiesTestMethods::createMediaCapabilitiesObject()
{
    m_mediaCapabilitiesFactory = IMediaCapabilitiesFactory::createFactory();
    EXPECT_NE(m_mediaCapabilitiesFactory, nullptr);

    m_mediaCapabilities = m_mediaCapabilitiesFactory->createMediaCapabilities();
    EXPECT_NE(m_mediaCapabilities, nullptr);
}

void MediaCapabilitiesTestMethods::destroyMediaCapabilitiesObject()
{
    m_mediaCapabilities.reset();
    m_mediaCapabilitiesFactory.reset();
}

void MediaCapabilitiesTestMethods::getSupportedAudioCapabilities()
{
    EXPECT_NE(m_mediaCapabilities, nullptr);
    // Set up mock to return audio capabilities with at least one capability
    EXPECT_CALL(*m_mediaCapabilitiesModuleMock, getSupportedAudioCapabilities(_, _, _, _))
        .WillOnce(Invoke(
            [](::google::protobuf::RpcController *, const ::firebolt::rialto::GetSupportedAudioCapabilitiesRequest *,
               ::firebolt::rialto::AudioCapabilities *response, ::google::protobuf::Closure *done)
            {
                // Populate response with at least one audio capability
                auto audioCapability = response->add_capabilities();
                // Set a basic PCM capability to make it non-empty
                auto pcmCapability = audioCapability->mutable_pcm();
                auto profileCapability = pcmCapability->mutable_base();
                profileCapability->set_max_channels(2);
                done->Run();
            }));
    // When the IPC service is available, getSupportedAudioCapabilities() should return non-empty capabilities
    auto audioCapabilities = m_mediaCapabilities->getSupportedAudioCapabilities();
    EXPECT_FALSE(audioCapabilities.capabilities.empty());
}

void MediaCapabilitiesTestMethods::getSupportedVideoCapabilities()
{
    EXPECT_NE(m_mediaCapabilities, nullptr);
    // Set up mock to return video capabilities with at least one capability
    EXPECT_CALL(*m_mediaCapabilitiesModuleMock, getSupportedVideoCapabilities(_, _, _, _))
        .WillOnce(Invoke(
            [](::google::protobuf::RpcController *, const ::firebolt::rialto::GetSupportedVideoCapabilitiesRequest *,
               ::firebolt::rialto::VideoCapabilities *response, ::google::protobuf::Closure *done)
            {
                // Populate response with at least one video capability
                auto videoCapability = response->add_capabilities();
                // Set a basic H264 codec capability to make it non-empty
                auto codecCapabilities = videoCapability->mutable_codec_capabilities();
                auto h264Capability = codecCapabilities->mutable_h264();
                auto h264Profile = h264Capability->add_profiles();
                h264Profile->set_type(::firebolt::rialto::VideoCapabilities::H264_PROFILE_HIGH);
                done->Run();
            }));
    // When the IPC service is available, getSupportedVideoCapabilities() should return non-empty capabilities
    auto videoCapabilities = m_mediaCapabilities->getSupportedVideoCapabilities();
    EXPECT_FALSE(videoCapabilities.capabilities.empty());
}

void MediaCapabilitiesTestMethods::getSupportedAudioCapabilitiesFailure()
{
    EXPECT_NE(m_mediaCapabilities, nullptr);
    // Set up mock to return empty audio capabilities (failure case)
    EXPECT_CALL(*m_mediaCapabilitiesModuleMock, getSupportedAudioCapabilities(_, _, _, _))
        .WillOnce(Invoke(
            [](::google::protobuf::RpcController *, const ::firebolt::rialto::GetSupportedAudioCapabilitiesRequest *,
               ::firebolt::rialto::AudioCapabilities *, ::google::protobuf::Closure *done)
            {
                // Don't populate response - return empty capabilities
                done->Run();
            }));
    // When the IPC service is not available, getSupportedAudioCapabilities() should return empty capabilities
    auto audioCapabilities = m_mediaCapabilities->getSupportedAudioCapabilities();
    EXPECT_TRUE(audioCapabilities.capabilities.empty());
}

void MediaCapabilitiesTestMethods::getSupportedVideoCapabilitiesFailure()
{
    EXPECT_NE(m_mediaCapabilities, nullptr);
    // Set up mock to return empty video capabilities (failure case)
    EXPECT_CALL(*m_mediaCapabilitiesModuleMock, getSupportedVideoCapabilities(_, _, _, _))
        .WillOnce(Invoke(
            [](::google::protobuf::RpcController *, const ::firebolt::rialto::GetSupportedVideoCapabilitiesRequest *,
               ::firebolt::rialto::VideoCapabilities *, ::google::protobuf::Closure *done)
            {
                // Don't populate response - return empty capabilities
                done->Run();
            }));
    // When the IPC service is not available, getSupportedVideoCapabilities() should return empty capabilities
    auto videoCapabilities = m_mediaCapabilities->getSupportedVideoCapabilities();
    EXPECT_TRUE(videoCapabilities.capabilities.empty());
}
} // namespace firebolt::rialto::client::ct
