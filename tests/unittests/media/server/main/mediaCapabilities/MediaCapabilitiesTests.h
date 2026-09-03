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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_TESTS_FIXTURE_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_TESTS_FIXTURE_H_

#include "IGstCapabilities.h"
#include "IGstCapabilitiesMock.h"
#include "MediaCapabilities.h"
#include <AudioDecoderCapabilities.h>
#include <VideoDecoderCapabilities.h>
#include <gtest/gtest.h>
#include <memory>
#include <optional>

using testing::StrictMock;

class MediaCapabilitiesTests : public testing::Test
{
public:
    MediaCapabilitiesTests();
    ~MediaCapabilitiesTests() override;

    void gstCapabilitiesWillNotBeQueried();
    void gstCapabilitiesWillReturnEmptyAudio();
    void gstCapabilitiesWillReturnEmptyVideo();

protected:
    std::shared_ptr<StrictMock<firebolt::rialto::server::IGstCapabilitiesMock>> m_gstCapabilitiesMock;
    std::shared_ptr<firebolt::rialto::server::MediaCapabilities> m_mediaCapabilities;
    
    firebolt::rialto::common::AudioDecoderCapabilities m_gstAudioCapabilities;
    firebolt::rialto::common::VideoDecoderCapabilities m_gstVideoCapabilities;
    
    const firebolt::rialto::common::AudioDecoderCapabilities m_preloadedAudio{"aac", "opus", {}};
    const firebolt::rialto::common::VideoDecoderCapabilities m_preloadedVideo{"h264", "h265", {}};
};

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_CAPABILITIES_TESTS_FIXTURE_H_
