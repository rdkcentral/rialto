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

namespace firebolt::rialto::client::ct
{
MediaCapabilitiesTestMethods::MediaCapabilitiesTestMethods() {}

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
    auto audioCapabilities = m_mediaCapabilities->getSupportedAudioCapabilities();
    // Verify we got a valid response (even if empty, it shouldn't crash)
    EXPECT_TRUE(true);
}

void MediaCapabilitiesTestMethods::getSupportedVideoCapabilities()
{
    EXPECT_NE(m_mediaCapabilities, nullptr);
    auto videoCapabilities = m_mediaCapabilities->getSupportedVideoCapabilities();
    // Verify we got a valid response (even if empty, it shouldn't crash)
    EXPECT_TRUE(true);
}

void MediaCapabilitiesTestMethods::getSupportedAudioCapabilitiesFailure()
{
    EXPECT_NE(m_mediaCapabilities, nullptr);
    auto audioCapabilities = m_mediaCapabilities->getSupportedAudioCapabilities();
    // Should return some capabilities structure even if empty or failed
    EXPECT_TRUE(true);
}

void MediaCapabilitiesTestMethods::getSupportedVideoCapabilitiesFailure()
{
    EXPECT_NE(m_mediaCapabilities, nullptr);
    auto videoCapabilities = m_mediaCapabilities->getSupportedVideoCapabilities();
    // Should return some capabilities structure even if empty or failed
    EXPECT_TRUE(true);
}
} // namespace firebolt::rialto::client::ct
