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

#include "MediaCapabilitiesFactory.h"
#include "IYamlCppWrapper.h"
#include "MediaCapabilities.h"

using rialto::servermanager::common::MediaCapabilities;

namespace rialto::servermanager::service
{
std::unique_ptr<IYamlCapabilities> createMediaCapabilities()
{
    auto factory = firebolt::rialto::wrappers::IYamlCppWrapperFactory::getFactory();
    if (!factory)
        return nullptr;
    auto wrapper = factory->createYamlCppWrapper();
    if (!wrapper)
        return nullptr;
    return std::make_unique<MediaCapabilities>(std::move(wrapper));
}

} // namespace rialto::servermanager::service
