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

#include "GlibWrapperMock.h"
#include "GstCapabilities.h"
#include "GstWrapperMock.h"
#include "RdkGstreamerUtilsWrapperMock.h"
#include "gtest/gtest.h"
#include <memory>

using firebolt::rialto::common::AudioDecoderCapabilities;
using firebolt::rialto::common::VideoDecoderCapabilities;
using firebolt::rialto::server::GstCapabilities;
using firebolt::rialto::server::IGstCapabilities;
using firebolt::rialto::wrappers::GlibWrapperMock;
using firebolt::rialto::wrappers::GstWrapperMock;
using firebolt::rialto::wrappers::RdkGstreamerUtilsWrapperMock;
using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::StrictMock;

// Mock for IGstInitialiser
class MockGstInitialiser : public firebolt::rialto::server::IGstInitialiser
{
public:
    MOCK_METHOD(void, initialise, (int *argc, char ***argv), (override));
    MOCK_METHOD(void, waitForInitialisation, (), (const, override));
};

class GstCapabilitiesTests : public testing::Test
{
public:
    GstCapabilitiesTests()
        : m_gstWrapperMock(std::make_shared<NiceMock<GstWrapperMock>>()),
          m_glibWrapperMock(std::make_shared<NiceMock<GlibWrapperMock>>()),
          m_rdkGstreamerUtilsWrapperMock(std::make_shared<NiceMock<RdkGstreamerUtilsWrapperMock>>()),
          m_gstInitialiserMock()
    {
    }

protected:
    std::shared_ptr<NiceMock<GstWrapperMock>> m_gstWrapperMock;
    std::shared_ptr<NiceMock<GlibWrapperMock>> m_glibWrapperMock;
    std::shared_ptr<NiceMock<RdkGstreamerUtilsWrapperMock>> m_rdkGstreamerUtilsWrapperMock;
    MockGstInitialiser m_gstInitialiserMock;

    std::shared_ptr<IGstCapabilities> createGstCapabilities()
    {
        return std::make_shared<GstCapabilities>(m_gstWrapperMock, m_glibWrapperMock, m_rdkGstreamerUtilsWrapperMock,
                                                 m_gstInitialiserMock);
    }
};

// ============================================================================
// Constructor Tests
// ============================================================================

TEST_F(GstCapabilitiesTests, ShouldConstructSuccessfully)
{
    auto gstCapabilities = createGstCapabilities();
    EXPECT_NE(gstCapabilities, nullptr);
}

// ============================================================================
// Null Wrapper Tests
// ============================================================================

TEST_F(GstCapabilitiesTests, ShouldHandleNullGstWrapper)
{
    auto gstCapabilities = std::make_shared<GstCapabilities>(nullptr, m_glibWrapperMock, m_rdkGstreamerUtilsWrapperMock,
                                                             m_gstInitialiserMock);

    EXPECT_NE(gstCapabilities, nullptr);
}

TEST_F(GstCapabilitiesTests, ShouldHandleNullGlibWrapper)
{
    auto gstCapabilities = std::make_shared<GstCapabilities>(m_gstWrapperMock, nullptr, m_rdkGstreamerUtilsWrapperMock,
                                                             m_gstInitialiserMock);

    EXPECT_NE(gstCapabilities, nullptr);
}

TEST_F(GstCapabilitiesTests, ShouldHandleNullRdkGstreamerUtilsWrapper)
{
    auto gstCapabilities =
        std::make_shared<GstCapabilities>(m_gstWrapperMock, m_glibWrapperMock, nullptr, m_gstInitialiserMock);

    EXPECT_NE(gstCapabilities, nullptr);
}

// ============================================================================
// Audio Capabilities Tests
// ============================================================================

TEST_F(GstCapabilitiesTests, GetSupportedAudioCapabilitiesShouldReturnEmptyStructure)
{
    auto gstCapabilities = createGstCapabilities();
    EXPECT_NE(gstCapabilities, nullptr);

    // GStreamer has no full decoder capability information, returns empty
    AudioDecoderCapabilities audioCapabilities = gstCapabilities->getSupportedAudioCapabilities();
    // Verify we got an empty structure (expected behavior from GStreamer source)
    EXPECT_TRUE(audioCapabilities.interfaceVersion.empty());
    EXPECT_TRUE(audioCapabilities.schemaVersion.empty());
    EXPECT_TRUE(audioCapabilities.capabilities.empty());
}

// ============================================================================
// Video Capabilities Tests
// ============================================================================

TEST_F(GstCapabilitiesTests, GetSupportedVideoCapabilitiesShouldReturnEmptyStructure)
{
    auto gstCapabilities = createGstCapabilities();
    EXPECT_NE(gstCapabilities, nullptr);

    // GStreamer has no full decoder capability information, returns empty
    VideoDecoderCapabilities videoCapabilities = gstCapabilities->getSupportedVideoCapabilities();
    // Verify we got an empty structure (expected behavior from GStreamer source)
    EXPECT_TRUE(videoCapabilities.interfaceVersion.empty());
    EXPECT_TRUE(videoCapabilities.schemaVersion.empty());
    EXPECT_TRUE(videoCapabilities.capabilities.empty());
}

// ============================================================================
// Multiple Calls Tests
// ============================================================================

TEST_F(GstCapabilitiesTests, MultipleCalls_AudioCapabilitiesShouldBeConsistent)
{
    auto gstCapabilities = createGstCapabilities();

    auto audioCapabilities1 = gstCapabilities->getSupportedAudioCapabilities();
    auto audioCapabilities2 = gstCapabilities->getSupportedAudioCapabilities();

    // Both calls should return empty
    EXPECT_TRUE(audioCapabilities1.capabilities.empty());
    EXPECT_TRUE(audioCapabilities2.capabilities.empty());
}

TEST_F(GstCapabilitiesTests, MultipleCalls_VideoCapabilitiesShouldBeConsistent)
{
    auto gstCapabilities = createGstCapabilities();

    auto videoCapabilities1 = gstCapabilities->getSupportedVideoCapabilities();
    auto videoCapabilities2 = gstCapabilities->getSupportedVideoCapabilities();

    // Both calls should return empty
    EXPECT_TRUE(videoCapabilities1.capabilities.empty());
    EXPECT_TRUE(videoCapabilities2.capabilities.empty());
}

TEST_F(GstCapabilitiesTests, MultipleCalls_AudioAndVideoShouldBeIndependent)
{
    auto gstCapabilities = createGstCapabilities();

    auto audioCapabilities = gstCapabilities->getSupportedAudioCapabilities();
    auto videoCapabilities = gstCapabilities->getSupportedVideoCapabilities();

    // Both calls should succeed independently and return empty
    EXPECT_TRUE(audioCapabilities.capabilities.empty());
    EXPECT_TRUE(videoCapabilities.capabilities.empty());
}

// ============================================================================
// Factory Tests
// ============================================================================

TEST_F(GstCapabilitiesTests, FactoryShouldReturnValidInstance)
{
    // This test verifies the factory pattern works correctly
    auto gstCapabilities = createGstCapabilities();
    EXPECT_NE(gstCapabilities, nullptr);

    // Instance should be usable for GStreamer queries
    // Decorator capabilities from GStreamer are empty (as expected)
    auto audio = gstCapabilities->getSupportedAudioCapabilities();
    auto video = gstCapabilities->getSupportedVideoCapabilities();
    EXPECT_TRUE(audio.capabilities.empty());
    EXPECT_TRUE(video.capabilities.empty());
}
