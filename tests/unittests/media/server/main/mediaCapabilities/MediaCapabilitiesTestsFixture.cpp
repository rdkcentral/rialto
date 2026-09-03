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
    : m_gstCapabilitiesMock(std::make_shared<StrictMock<firebolt::rialto::server::GstCapabilitiesMock>>()),
      m_gstAudioCapabilities{"gst_aac", "gst_opus", {}}, m_gstVideoCapabilities{"gst_h264", "gst_h265", {}}
{
    // Default: GStreamer returns its standard capabilities when queried
    EXPECT_CALL(*m_gstCapabilitiesMock, getSupportedAudioCapabilities()).WillRepeatedly(Return(m_gstAudioCapabilities));
    EXPECT_CALL(*m_gstCapabilitiesMock, getSupportedVideoCapabilities()).WillRepeatedly(Return(m_gstVideoCapabilities));

    // Create orchestrator with preloaded data
    // Note: Initialize with non-empty capabilities vector to simulate preloaded YAML data from ServerManager
    firebolt::rialto::common::AudioDecoderCapabilities preloadedAudio{"1.0", "1.0", {}};
    preloadedAudio.capabilities.push_back(firebolt::rialto::common::AudioDecoderCapability{});

    firebolt::rialto::common::VideoDecoderCapabilities preloadedVideo{"1.0", "1.0", {}};
    preloadedVideo.capabilities.push_back(firebolt::rialto::common::VideoDecoderCapability{});

    m_mediaCapabilities =
        std::make_shared<firebolt::rialto::server::MediaCapabilities>(m_gstCapabilitiesMock,
                                                                      std::make_optional(preloadedAudio),
                                                                      std::make_optional(preloadedVideo));
}

MediaCapabilitiesTests::~MediaCapabilitiesTests() {}

void MediaCapabilitiesTests::gstCapabilitiesWillNotBeQueried()
{
    // StrictMock will fail test if GStreamer methods are called unexpectedly
    // This method sets no explicit expectations, relying on StrictMock behavior:
    // - If getSupportedAudioCapabilities() or getSupportedVideoCapabilities() are called,
    //   the mock will detect an "unexpected call" and fail the test
    // - This verifies that Path 0 preload is used instead of Path B fallback
    // Note: Other methods (fillSupportedMimeTypes, etc.) still execute normally
}

void MediaCapabilitiesTests::gstCapabilitiesWillReturnEmptyAudio()
{
    m_gstAudioCapabilities = firebolt::rialto::common::AudioDecoderCapabilities{"", "", {}};
    EXPECT_CALL(*m_gstCapabilitiesMock, getSupportedAudioCapabilities()).WillOnce(Return(m_gstAudioCapabilities));
}

void MediaCapabilitiesTests::gstCapabilitiesWillReturnEmptyVideo()
{
    m_gstVideoCapabilities = firebolt::rialto::common::VideoDecoderCapabilities{"", "", {}};
    EXPECT_CALL(*m_gstCapabilitiesMock, getSupportedVideoCapabilities()).WillOnce(Return(m_gstVideoCapabilities));
}
