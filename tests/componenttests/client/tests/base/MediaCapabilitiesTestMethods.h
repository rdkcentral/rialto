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

#ifndef FIREBOLT_RIALTO_CLIENT_CT_MEDIA_CAPABILITIES_TEST_METHODS_H_
#define FIREBOLT_RIALTO_CLIENT_CT_MEDIA_CAPABILITIES_TEST_METHODS_H_

#include "AudioDecoderCapabilities.h"
#include "IMediaCapabilities.h"
#include "VideoDecoderCapabilities.h"
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;

namespace firebolt::rialto::client::ct
{
class MediaCapabilitiesTestMethods
{
public:
    MediaCapabilitiesTestMethods();
    virtual ~MediaCapabilitiesTestMethods();

protected:
    // Objects
    std::shared_ptr<IMediaCapabilitiesFactory> m_mediaCapabilitiesFactory;
    std::unique_ptr<IMediaCapabilities> m_mediaCapabilities;

    // Api methods
    void createMediaCapabilitiesObject();
    void destroyMediaCapabilitiesObject();
    void getSupportedAudioCapabilities();
    void getSupportedVideoCapabilities();
    void getSupportedAudioCapabilitiesFailure();
    void getSupportedVideoCapabilitiesFailure();
};
} // namespace firebolt::rialto::client::ct

#endif // FIREBOLT_RIALTO_CLIENT_CT_MEDIA_CAPABILITIES_TEST_METHODS_H_
