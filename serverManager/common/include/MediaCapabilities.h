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

#ifndef RIALTO_SERVERMANAGER_COMMON_MEDIA_CAPABILITIES_H_
#define RIALTO_SERVERMANAGER_COMMON_MEDIA_CAPABILITIES_H_

#include "IMediaCapabilities.h"
#include "IYamlCppWrapper.h"
#include <memory>

namespace rialto::servermanager::common
{

class MediaCapabilities : public rialto::servermanager::service::IYamlCapabilities
{
public:
    explicit MediaCapabilities(std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper> yamlCppWrapper);
    ~MediaCapabilities() override = default;

    firebolt::rialto::common::DecoderCapabilitiesStatus
    getAudioDecoderCapabilities(firebolt::rialto::common::AudioDecoderCapabilities &capabilities) override;

    firebolt::rialto::common::DecoderCapabilitiesStatus
    getVideoDecoderCapabilities(firebolt::rialto::common::VideoDecoderCapabilities &capabilities) override;

private:
    std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper> m_yamlCppWrapper;
};

} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_MEDIA_CAPABILITIES_H_
