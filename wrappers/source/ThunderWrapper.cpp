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

#include "ThunderWrapper.h"
#include <core/Portability.h>

namespace firebolt::rialto::wrappers
{
std::shared_ptr<IThunderWrapper> ThunderWrapperFactory::getThunderWrapper()
{
    return std::make_shared<ThunderWrapper>();
}

const char *ThunderWrapper::errorToString(std::uint32_t errorCode) const
{
    return WPEFramework::Core::ErrorToString(errorCode);
}

bool ThunderWrapper::isSuccessful(std::uint32_t errorCode) const
{
    return errorCode == WPEFramework::Core::ERROR_NONE;
}
} // namespace firebolt::rialto::wrappers
