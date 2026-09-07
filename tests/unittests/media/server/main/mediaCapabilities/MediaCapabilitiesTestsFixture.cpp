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

#include "MediaCapabilitiesTests.h"

using testing::Return;

MediaCapabilitiesTests::MediaCapabilitiesTests()
    : m_gstAudioCapabilities{"gst_aac", "gst_opus", {}}, m_gstVideoCapabilities{"gst_h264", "gst_h265", {}}
{
    // Populate GStreamer mock capabilities with capability entries
    m_gstAudioCapabilities.capabilities.push_back(firebolt::rialto::common::AudioDecoderCapability{});
    m_gstVideoCapabilities.capabilities.push_back(firebolt::rialto::common::VideoDecoderCapability{});

    // Create mock and inject into MediaCapabilities for ownership
    // The mock is created as unique_ptr so MediaCapabilities owns it exclusively
    auto gstMockUnique = std::make_unique<StrictMock<firebolt::rialto::server::GstCapabilitiesMock>>();

    // Store raw pointer reference for setting expectations in tests
    // This allows tests to set expectations on the SAME instance that's injected
    m_gstCapabilitiesMock = gstMockUnique.get();

    // Set default behavior on the mock that will be owned by MediaCapabilities
    ON_CALL(*gstMockUnique, getSupportedAudioCapabilities()).WillByDefault(Return(m_gstAudioCapabilities));
    ON_CALL(*gstMockUnique, getSupportedVideoCapabilities()).WillByDefault(Return(m_gstVideoCapabilities));

    m_mediaCapabilities = std::make_shared<firebolt::rialto::server::MediaCapabilities>(std::move(gstMockUnique));
}

MediaCapabilitiesTests::~MediaCapabilitiesTests() {}

void MediaCapabilitiesTests::gstCapabilitiesWillBeQueried()
{
    // Allow queries to GStreamer capabilities with default return values.
    // Tests that need fallback behavior should call this method.
    // m_gstCapabilitiesMock points to the same instance injected into MediaCapabilities
    EXPECT_CALL(*m_gstCapabilitiesMock, getSupportedAudioCapabilities()).WillRepeatedly(Return(m_gstAudioCapabilities));
    EXPECT_CALL(*m_gstCapabilitiesMock, getSupportedVideoCapabilities()).WillRepeatedly(Return(m_gstVideoCapabilities));
}
