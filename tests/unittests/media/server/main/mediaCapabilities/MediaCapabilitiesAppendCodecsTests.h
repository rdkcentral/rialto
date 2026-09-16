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

#ifndef MEDIA_CAPABILITIES_APPEND_CODECS_TESTS_H_
#define MEDIA_CAPABILITIES_APPEND_CODECS_TESTS_H_

#include "GstCapabilitiesMock.h"
#include "IGstCapabilities.h"
#include "MediaCapabilitiesServerInternal.h"
#include <AudioDecoderCapabilities.h>
#include <VideoDecoderCapabilities.h>
#include <gtest/gtest.h>
#include <memory>
#include <optional>

using testing::NiceMock;

/**
 * @brief Test fixture for MediaCapabilitiesServerInternal append codec functionality
 *
 * Tests the new append logic that combines YAML preloaded capabilities with
 * GStreamer discovered codecs to provide enhanced codec support.
 */
class MediaCapabilitiesAppendCodecsTests : public testing::Test
{
public:
    MediaCapabilitiesAppendCodecsTests() = default;
    ~MediaCapabilitiesAppendCodecsTests() override = default;

protected:
    /**
     * @brief Helper to create empty audio capabilities
     */
    firebolt::rialto::common::AudioDecoderCapabilities getEmptyAudioCapabilities() const
    {
        return firebolt::rialto::common::AudioDecoderCapabilities{"empty_v1", "1.0", {}};
    }

    /**
     * @brief Helper to create empty video capabilities
     */
    firebolt::rialto::common::VideoDecoderCapabilities getEmptyVideoCapabilities() const
    {
        return firebolt::rialto::common::VideoDecoderCapabilities{"empty_v1", "1.0", {}};
    }
};

#endif // MEDIA_CAPABILITIES_APPEND_CODECS_TESTS_H_
