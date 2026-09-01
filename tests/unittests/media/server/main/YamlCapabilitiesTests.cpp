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

#include "YamlCapabilities.h"
#include "YamlCppWrapperMock.h"
#include "gtest/gtest.h"

using firebolt::rialto::DecoderCapabilitiesStatus;
using firebolt::rialto::common::AudioDecoderCapabilities;
using firebolt::rialto::common::VideoDecoderCapabilities;
using firebolt::rialto::server::YamlCapabilities;
using firebolt::rialto::wrappers::YamlCppWrapperMock;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgReferee;
using testing::StrictMock;

class YamlCapabilitiesTests : public testing::Test
{
public:
    YamlCapabilitiesTests() : m_yamlCppWrapperMock(std::make_shared<StrictMock<YamlCppWrapperMock>>())
    {
        m_sut = std::make_unique<YamlCapabilities>(m_yamlCppWrapperMock);
    }

protected:
    std::shared_ptr<StrictMock<YamlCppWrapperMock>> m_yamlCppWrapperMock;
    std::unique_ptr<YamlCapabilities> m_sut;
};

// ============================================================================
// Audio Decoder Capabilities Tests
// ============================================================================

TEST_F(YamlCapabilitiesTests, GetAudioDecoderCapabilitiesShouldReturnOkStatus)
{
    AudioDecoderCapabilities returnedCapabilities;
    returnedCapabilities.interfaceVersion = "1.0";
    returnedCapabilities.schemaVersion = "2.0";

    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(returnedCapabilities), Return(DecoderCapabilitiesStatus::OK)));

    AudioDecoderCapabilities capabilities;
    auto status = m_sut->getAudioDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(capabilities.interfaceVersion, "1.0");
    EXPECT_EQ(capabilities.schemaVersion, "2.0");
}

TEST_F(YamlCapabilitiesTests, GetAudioDecoderCapabilitiesShouldReturnConfigNotFoundStatus)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::CONFIG_NOT_FOUND));

    AudioDecoderCapabilities capabilities;
    auto status = m_sut->getAudioDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::CONFIG_NOT_FOUND);
}

TEST_F(YamlCapabilitiesTests, GetAudioDecoderCapabilitiesShouldReturnSchemaValidationFailedStatus)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED));

    AudioDecoderCapabilities capabilities;
    auto status = m_sut->getAudioDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED);
}

TEST_F(YamlCapabilitiesTests, GetAudioDecoderCapabilitiesShouldReturnInternalErrorStatus)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::INTERNAL_ERROR));

    AudioDecoderCapabilities capabilities;
    auto status = m_sut->getAudioDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::INTERNAL_ERROR);
}

TEST_F(YamlCapabilitiesTests, GetAudioDecoderCapabilitiesShouldPopulateOutputCapabilities)
{
    AudioDecoderCapabilities returnedCapabilities;
    returnedCapabilities.interfaceVersion = "3.0";
    returnedCapabilities.schemaVersion = "4.0";

    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(returnedCapabilities), Return(DecoderCapabilitiesStatus::OK)));

    AudioDecoderCapabilities capabilities;
    auto status = m_sut->getAudioDecoderCapabilities(capabilities);

    ASSERT_EQ(status, DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(capabilities.interfaceVersion, "3.0");
    EXPECT_EQ(capabilities.schemaVersion, "4.0");
}

// ============================================================================
// Video Decoder Capabilities Tests
// ============================================================================

TEST_F(YamlCapabilitiesTests, GetVideoDecoderCapabilitiesShouldReturnOkStatus)
{
    VideoDecoderCapabilities returnedCapabilities;
    returnedCapabilities.interfaceVersion = "5.0";
    returnedCapabilities.schemaVersion = "6.0";

    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(returnedCapabilities), Return(DecoderCapabilitiesStatus::OK)));

    VideoDecoderCapabilities capabilities;
    auto status = m_sut->getVideoDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(capabilities.interfaceVersion, "5.0");
    EXPECT_EQ(capabilities.schemaVersion, "6.0");
}

TEST_F(YamlCapabilitiesTests, GetVideoDecoderCapabilitiesShouldReturnConfigNotFoundStatus)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::CONFIG_NOT_FOUND));

    VideoDecoderCapabilities capabilities;
    auto status = m_sut->getVideoDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::CONFIG_NOT_FOUND);
}

TEST_F(YamlCapabilitiesTests, GetVideoDecoderCapabilitiesShouldReturnSchemaValidationFailedStatus)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED));

    VideoDecoderCapabilities capabilities;
    auto status = m_sut->getVideoDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED);
}

TEST_F(YamlCapabilitiesTests, GetVideoDecoderCapabilitiesShouldReturnInternalErrorStatus)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::INTERNAL_ERROR));

    VideoDecoderCapabilities capabilities;
    auto status = m_sut->getVideoDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::INTERNAL_ERROR);
}

TEST_F(YamlCapabilitiesTests, GetVideoDecoderCapabilitiesShouldPopulateOutputCapabilities)
{
    VideoDecoderCapabilities returnedCapabilities;
    returnedCapabilities.interfaceVersion = "7.0";
    returnedCapabilities.schemaVersion = "8.0";

    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(returnedCapabilities), Return(DecoderCapabilitiesStatus::OK)));

    VideoDecoderCapabilities capabilities;
    auto status = m_sut->getVideoDecoderCapabilities(capabilities);

    ASSERT_EQ(status, DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(capabilities.interfaceVersion, "7.0");
    EXPECT_EQ(capabilities.schemaVersion, "8.0");
}

// ============================================================================
// Null Wrapper Tests
// ============================================================================

TEST_F(YamlCapabilitiesTests, GetAudioDecoderCapabilitiesShouldReturnInternalErrorWhenWrapperIsNull)
{
    // Create instance with null wrapper
    auto sut = std::make_unique<YamlCapabilities>(nullptr);

    AudioDecoderCapabilities capabilities;
    auto status = sut->getAudioDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::INTERNAL_ERROR);
}

TEST_F(YamlCapabilitiesTests, GetVideoDecoderCapabilitiesShouldReturnInternalErrorWhenWrapperIsNull)
{
    // Create instance with null wrapper
    auto sut = std::make_unique<YamlCapabilities>(nullptr);

    VideoDecoderCapabilities capabilities;
    auto status = sut->getVideoDecoderCapabilities(capabilities);

    EXPECT_EQ(status, DecoderCapabilitiesStatus::INTERNAL_ERROR);
}

// ============================================================================
// Multiple Calls Tests
// ============================================================================

TEST_F(YamlCapabilitiesTests, MultipleCalls_GetAudioDecoderCapabilitiesShouldReturnConsistentResults)
{
    AudioDecoderCapabilities firstCall;
    firstCall.interfaceVersion = "1.0";
    AudioDecoderCapabilities secondCall;
    secondCall.interfaceVersion = "2.0";

    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(firstCall), Return(DecoderCapabilitiesStatus::OK)))
        .WillOnce(DoAll(SetArgReferee<0>(secondCall), Return(DecoderCapabilitiesStatus::OK)));

    AudioDecoderCapabilities capabilities1;
    auto status1 = m_sut->getAudioDecoderCapabilities(capabilities1);
    ASSERT_EQ(status1, DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(capabilities1.interfaceVersion, "1.0");

    AudioDecoderCapabilities capabilities2;
    auto status2 = m_sut->getAudioDecoderCapabilities(capabilities2);
    ASSERT_EQ(status2, DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(capabilities2.interfaceVersion, "2.0");
}

TEST_F(YamlCapabilitiesTests, MultipleCalls_AudioAndVideoShouldBeIndependent)
{
    AudioDecoderCapabilities audioCapabilities;
    audioCapabilities.interfaceVersion = "audio-1.0";
    VideoDecoderCapabilities videoCapabilities;
    videoCapabilities.interfaceVersion = "video-2.0";

    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(audioCapabilities), Return(DecoderCapabilitiesStatus::OK)));
    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(videoCapabilities), Return(DecoderCapabilitiesStatus::OK)));

    AudioDecoderCapabilities audio;
    auto audioStatus = m_sut->getAudioDecoderCapabilities(audio);
    ASSERT_EQ(audioStatus, DecoderCapabilitiesStatus::OK);

    VideoDecoderCapabilities video;
    auto videoStatus = m_sut->getVideoDecoderCapabilities(video);
    ASSERT_EQ(videoStatus, DecoderCapabilitiesStatus::OK);

    EXPECT_EQ(audio.interfaceVersion, "audio-1.0");
    EXPECT_EQ(video.interfaceVersion, "video-2.0");
}
