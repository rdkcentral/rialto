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

#ifndef FIREBOLT_RIALTO_WRAPPERS_THUNDER_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_THUNDER_WRAPPER_H_

#include "IThunderWrapper.h"
#include <memory>

namespace firebolt::rialto::wrappers
{
class ThunderWrapperFactory : public IThunderWrapperFactory
{
public:
    ThunderWrapperFactory() = default;
    ~ThunderWrapperFactory() override = default;

    std::shared_ptr<IThunderWrapper> getThunderWrapper() override;
};

class ThunderWrapper : public IThunderWrapper
{
public:
    ThunderWrapper() = default;
    ~ThunderWrapper() override = default;

    const char *errorToString(std::uint32_t errorCode) const override;
    bool isSuccessful(std::uint32_t errorCode) const override;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_THUNDER_WRAPPER_H_
