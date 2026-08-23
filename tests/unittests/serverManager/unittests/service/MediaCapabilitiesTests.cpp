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

#include "MediaCapabilities.h"
#include "YamlCppWrapperMock.h"
#include "gtest/gtest.h"

using firebolt::rialto::DecoderCapabilitiesStatus;
using firebolt::rialto::common::AudioDecoderCapabilities;
using firebolt::rialto::common::VideoDecoderCapabilities;
using firebolt::rialto::wrappers::YamlCppWrapperMock;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgReferee;
using testing::StrictMock;
using namespace rialto::servermanager::service;

class MediaCapabilitiesTests : public testing::Test
{
public:
    MediaCapabilitiesTests() : m_yamlCppWrapperMock(std::make_shared<StrictMock<YamlCppWrapperMock>>())
    {
        m_sut = std::make_unique<MediaCapabilities>(m_yamlCppWrapperMock);
    }

protected:
    std::shared_ptr<StrictMock<YamlCppWrapperMock>> m_yamlCppWrapperMock;
    std::unique_ptr<MediaCapabilities> m_sut;
};

TEST_F(MediaCapabilitiesTests, GetAudioDecoderCapabilitiesShouldReturnOk)
{
    AudioDecoderCapabilities returnedCapabilities;
    returnedCapabilities.interfaceVersion = "1.0";
    returnedCapabilities.schemaVersion = "2.0";
    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(returnedCapabilities), Return(DecoderCapabilitiesStatus::OK)));

    AudioDecoderCapabilities capabilities;
    EXPECT_EQ(m_sut->getAudioDecoderCapabilities(capabilities), DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(capabilities.interfaceVersion, "1.0");
    EXPECT_EQ(capabilities.schemaVersion, "2.0");
}

TEST_F(MediaCapabilitiesTests, GetAudioDecoderCapabilitiesShouldReturnConfigNotFound)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::CONFIG_NOT_FOUND));

    AudioDecoderCapabilities capabilities;
    EXPECT_EQ(m_sut->getAudioDecoderCapabilities(capabilities), DecoderCapabilitiesStatus::CONFIG_NOT_FOUND);
}

TEST_F(MediaCapabilitiesTests, GetAudioDecoderCapabilitiesShouldReturnSchemaValidationFailed)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED));

    AudioDecoderCapabilities capabilities;
    EXPECT_EQ(m_sut->getAudioDecoderCapabilities(capabilities), DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED);
}

TEST_F(MediaCapabilitiesTests, GetAudioDecoderCapabilitiesShouldReturnInternalError)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getAudioDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::INTERNAL_ERROR));

    AudioDecoderCapabilities capabilities;
    EXPECT_EQ(m_sut->getAudioDecoderCapabilities(capabilities), DecoderCapabilitiesStatus::INTERNAL_ERROR);
}

TEST_F(MediaCapabilitiesTests, GetVideoDecoderCapabilitiesShouldReturnOk)
{
    VideoDecoderCapabilities returnedCapabilities;
    returnedCapabilities.interfaceVersion = "3.0";
    returnedCapabilities.schemaVersion = "4.0";
    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(returnedCapabilities), Return(DecoderCapabilitiesStatus::OK)));

    VideoDecoderCapabilities capabilities;
    EXPECT_EQ(m_sut->getVideoDecoderCapabilities(capabilities), DecoderCapabilitiesStatus::OK);
    EXPECT_EQ(capabilities.interfaceVersion, "3.0");
    EXPECT_EQ(capabilities.schemaVersion, "4.0");
}

TEST_F(MediaCapabilitiesTests, GetVideoDecoderCapabilitiesShouldReturnConfigNotFound)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::CONFIG_NOT_FOUND));

    VideoDecoderCapabilities capabilities;
    EXPECT_EQ(m_sut->getVideoDecoderCapabilities(capabilities), DecoderCapabilitiesStatus::CONFIG_NOT_FOUND);
}

TEST_F(MediaCapabilitiesTests, GetVideoDecoderCapabilitiesShouldReturnSchemaValidationFailed)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED));

    VideoDecoderCapabilities capabilities;
    EXPECT_EQ(m_sut->getVideoDecoderCapabilities(capabilities), DecoderCapabilitiesStatus::SCHEMA_VALIDATION_FAILED);
}

TEST_F(MediaCapabilitiesTests, GetVideoDecoderCapabilitiesShouldReturnInternalError)
{
    EXPECT_CALL(*m_yamlCppWrapperMock, getVideoDecoderCapabilities(_))
        .WillOnce(Return(DecoderCapabilitiesStatus::INTERNAL_ERROR));

    VideoDecoderCapabilities capabilities;
    EXPECT_EQ(m_sut->getVideoDecoderCapabilities(capabilities), DecoderCapabilitiesStatus::INTERNAL_ERROR);
}
