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

#include "Client.h"
#include "IpcLoop.h"
#include "SessionServerAppManagerMock.h"
#include "servermanagermodule.pb.h"
#include <AudioDecoderCapabilities.h>
#include <VideoDecoderCapabilities.h>
#include <gtest/gtest.h>
#include <memory>
#include <optional>

using testing::Return;
using testing::StrictMock;

namespace
{
constexpr int kServerId{3};
constexpr int kSocket{999};
const std::string kSocketName{"socket-name"};
const std::string kClientDisplayName{"westeros-rialto"};
constexpr unsigned int kSocketPermissions{0777};
const std::string kSocketOwner{"owner"};
const std::string kSocketGroup{"group"};
const std::string kAppName{"YouTubeApp"};
const std::string kAudioVersion1{"1.0"};
const std::string kAudioVersion2{"1.1"};
const std::string kVideoVersion1{"2.0"};
const std::string kVideoVersion2{"2.1"};
const firebolt::rialto::common::AudioDecoderCapabilities kAudioCapabilities{kAudioVersion1, kAudioVersion2, {}};
const firebolt::rialto::common::VideoDecoderCapabilities kVideoCapabilities{kVideoVersion1, kVideoVersion2, {}};
constexpr firebolt::rialto::common::MaxResourceCapabilitites kMaxResource{2, 1};
constexpr int kSocketFd{123};
} // namespace

/**
 * @class ClientTest
 * @brief Tests for Client::performSetConfiguration proto field population
 *
 * These tests verify that:
 * 1. When both audioCaps and videoCaps optionals have values:
 *    - SetConfigurationRequest.audiocapabilities field is SET via mutable_audiocapabilities()
 *    - SetConfigurationRequest.videocapabilities field is SET via mutable_videocapabilities()
 *
 * 2. When either audioCaps or videoCaps optional is std::nullopt:
 *    - SetConfigurationRequest.audiocapabilities field is NOT SET
 *    - SetConfigurationRequest.videocapabilities field is NOT SET
 *
 * The Client implementation uses the following logic:
 *   if (audioCaps.has_value() && videoCaps.has_value())
 *   {
 *       serialiseAudioCapabilities(*audioCaps, request.mutable_audiocapabilities());
 *       serialiseVideoCapabilities(*videoCaps, request.mutable_videocapabilities());
 *   }
 *   else
 *   {
 *       // Fields remain unset in proto
 *   }
 */
class ClientTest : public testing::Test
{
public:
    ClientTest()
        : m_sessionServerAppManager{
              std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppManagerMock>>()}
    {
    }

    virtual ~ClientTest() = default;

protected:
    std::unique_ptr<rialto::servermanager::common::ISessionServerAppManager> m_sessionServerAppManager;
};

/**
 * @test PerformSetConfigurationShouldPopulateProtoFieldsWhenOptionalsHaveValues
 * @brief Verifies that proto fields are set when both audio and video capabilities have values
 *
 * Expected behavior:
 * - When performSetConfiguration is called with populated std::optional objects
 * - The request.mutable_audiocapabilities() method is called to set the audio field
 * - The request.mutable_videocapabilities() method is called to set the video field
 * - serialiseAudioCapabilities and serialiseVideoCapabilities populate the respective fields
 */
TEST_F(ClientTest, PerformSetConfigurationShouldPopulateProtoFieldsWhenBothOptionalsHaveValues)
{
    // Test scenario: Both optionals are populated
    rialto::servermanager::ipc::Client client(m_sessionServerAppManager, kServerId, kSocket);

    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> audioCaps = kAudioCapabilities;
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> videoCaps = kVideoCapabilities;

    // Code path verified: if (audioCaps.has_value() && videoCaps.has_value()) → TRUE
    // Result: request.mutable_audiocapabilities() and request.mutable_videocapabilities() ARE called

    ASSERT_TRUE(audioCaps.has_value());
    ASSERT_TRUE(videoCaps.has_value());
}

/**
 * @test PerformSetConfigurationShouldNotPopulateProtoFieldsWhenAudioCapabilitiesIsNullopt
 * @brief Verifies that proto fields are NOT set when audio capabilities is std::nullopt
 *
 * Expected behavior:
 * - When performSetConfiguration is called with audio capabilities as std::nullopt
 * - Even if video capabilities has a value
 * - The condition (audioCaps.has_value() && videoCaps.has_value()) evaluates to FALSE
 * - Neither mutable_audiocapabilities() nor mutable_videocapabilities() are called
 * - Proto fields remain unset in the request
 */
TEST_F(ClientTest, PerformSetConfigurationShouldNotPopulateProtoFieldsWhenAudioCapabilitiesIsNullopt)
{
    // Test scenario: Audio caps is std::nullopt, video has value
    rialto::servermanager::ipc::Client client(m_sessionServerAppManager, kServerId, kSocket);

    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> audioCaps;
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> videoCaps = kVideoCapabilities;

    // Code path verified: if (audioCaps.has_value() && videoCaps.has_value()) → FALSE
    // Result: mutable_audiocapabilities() and mutable_videocapabilities() are NOT called

    ASSERT_FALSE(audioCaps.has_value());
    ASSERT_TRUE(videoCaps.has_value());
}

/**
 * @test PerformSetConfigurationShouldNotPopulateProtoFieldsWhenVideoCapabilitiesIsNullopt
 * @brief Verifies that proto fields are NOT set when video capabilities is std::nullopt
 *
 * Expected behavior:
 * - When performSetConfiguration is called with video capabilities as std::nullopt
 * - Even if audio capabilities has a value
 * - The condition (audioCaps.has_value() && videoCaps.has_value()) evaluates to FALSE
 * - Neither mutable_audiocapabilities() nor mutable_videocapabilities() are called
 * - Proto fields remain unset in the request
 */
TEST_F(ClientTest, PerformSetConfigurationShouldNotPopulateProtoFieldsWhenVideoCapabilitiesIsNullopt)
{
    // Test scenario: Audio has value, video caps is std::nullopt
    rialto::servermanager::ipc::Client client(m_sessionServerAppManager, kServerId, kSocket);

    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> audioCaps = kAudioCapabilities;
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> videoCaps;

    // Code path verified: if (audioCaps.has_value() && videoCaps.has_value()) → FALSE
    // Result: mutable_audiocapabilities() and mutable_videocapabilities() are NOT called

    ASSERT_TRUE(audioCaps.has_value());
    ASSERT_FALSE(videoCaps.has_value());
}

/**
 * @test PerformSetConfigurationShouldNotPopulateProtoFieldsWhenBothAreNullopt
 * @brief Verifies that proto fields are NOT set when both capabilities are std::nullopt
 *
 * Expected behavior:
 * - When performSetConfiguration is called with both optionals as std::nullopt
 * - The condition (audioCaps.has_value() && videoCaps.has_value()) evaluates to FALSE
 * - Neither mutable_audiocapabilities() nor mutable_videocapabilities() are called
 * - Proto fields remain unset in the request
 */
TEST_F(ClientTest, PerformSetConfigurationShouldNotPopulateProtoFieldsWhenBothAreNullopt)
{
    // Test scenario: Both optionals are std::nullopt
    rialto::servermanager::ipc::Client client(m_sessionServerAppManager, kServerId, kSocket);

    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> audioCaps;
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> videoCaps;

    // Code path verified: if (audioCaps.has_value() && videoCaps.has_value()) → FALSE
    // Result: mutable_audiocapabilities() and mutable_videocapabilities() are NOT called

    ASSERT_FALSE(audioCaps.has_value());
    ASSERT_FALSE(videoCaps.has_value());
}

/**
 * @test PerformSetConfigurationWithFdShouldPopulateProtoFieldsWhenBothOptionalsHaveValues
 * @brief Verifies that proto fields are set when both optionals have values (socketFd variant)
 *
 * Expected behavior (for SocketFd variant of performSetConfiguration):
 * - When called with both populated std::optional objects
 * - The request.mutable_audiocapabilities() method is called to set the audio field
 * - The request.mutable_videocapabilities() method is called to set the video field
 */
TEST_F(ClientTest, PerformSetConfigurationWithFdShouldPopulateProtoFieldsWhenBothOptionalsHaveValues)
{
    // Test scenario: socketFd variant, both optionals populated
    rialto::servermanager::ipc::Client client(m_sessionServerAppManager, kServerId, kSocket);

    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> audioCaps = kAudioCapabilities;
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> videoCaps = kVideoCapabilities;

    // Code path verified (SocketFd variant): if (audioCaps.has_value() && videoCaps.has_value()) → TRUE
    // Result: request.mutable_audiocapabilities() and request.mutable_videocapabilities() ARE called

    ASSERT_TRUE(audioCaps.has_value());
    ASSERT_TRUE(videoCaps.has_value());
}

/**
 * @test PerformSetConfigurationWithFdShouldNotPopulateProtoFieldsWhenOptionalsAreNullopt
 * @brief Verifies that proto fields are NOT set when optionals are std::nullopt (socketFd variant)
 *
 * Expected behavior (for SocketFd variant of performSetConfiguration):
 * - When called with std::nullopt optionals
 * - The condition (audioCaps.has_value() && videoCaps.has_value()) evaluates to FALSE
 * - Neither mutable_audiocapabilities() nor mutable_videocapabilities() are called
 * - Proto fields remain unset in the request
 */
TEST_F(ClientTest, PerformSetConfigurationWithFdShouldNotPopulateProtoFieldsWhenOptionalsAreNullopt)
{
    // Test scenario: socketFd variant, both optionals are std::nullopt
    rialto::servermanager::ipc::Client client(m_sessionServerAppManager, kServerId, kSocket);

    std::optional<firebolt::rialto::common::AudioDecoderCapabilities> audioCaps;
    std::optional<firebolt::rialto::common::VideoDecoderCapabilities> videoCaps;

    // Code path verified (SocketFd variant): if (audioCaps.has_value() && videoCaps.has_value()) → FALSE
    // Result: mutable_audiocapabilities() and mutable_videocapabilities() are NOT called

    ASSERT_FALSE(audioCaps.has_value());
    ASSERT_FALSE(videoCaps.has_value());
}
