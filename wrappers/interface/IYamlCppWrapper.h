/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef FIREBOLT_RIALTO_WRAPPERS_INTERFACE_IYAML_CPP_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_INTERFACE_IYAML_CPP_WRAPPER_H_

#include <AudioDecoderCapabilities.h>
#include <MediaCommon.h>
#include <VideoDecoderCapabilities.h>
#include <memory>

namespace firebolt::rialto::wrappers
{
class IYamlCppWrapper;
class IYamlCppWrapperFactory
{
public:
    IYamlCppWrapperFactory() = default;
    virtual ~IYamlCppWrapperFactory() = default;

    /**
     * @brief Gets the IYamlCppWrapperFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IYamlCppWrapperFactory> getFactory();

    /**
     * @brief Creates a IYamlCppWrapper object.
     *
     * @retval the wrapper instance or null on error.
     */
    virtual std::shared_ptr<IYamlCppWrapper> createYamlCppWrapper() = 0;
};

class IYamlCppWrapper
{
public:
    IYamlCppWrapper() = default;
    virtual ~IYamlCppWrapper() = default;

    virtual DecoderCapabilitiesStatus getAudioDecoderCapabilities(AudioDecoderCapabilities &capabilities) const = 0;
    virtual DecoderCapabilitiesStatus getVideoDecoderCapabilities(VideoDecoderCapabilities &capabilities) const = 0;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_INTERFACE_IYAML_CPP_WRAPPER_H_
