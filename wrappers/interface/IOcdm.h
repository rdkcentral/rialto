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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_OCDM_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_OCDM_H_

#include <MediaCommon.h>
#include <memory>
#include <stdint.h>
#include <string>

namespace firebolt::rialto::wrappers
{
class IOcdm;

/**
 * @brief IOcdm factory class, for the IOcdm singleton object.
 */
class IOcdmFactory
{
public:
    IOcdmFactory() = default;
    virtual ~IOcdmFactory() = default;

    /**
     * @brief Gets the IOcdmFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IOcdmFactory> createFactory();

    /**
     * @brief Gets a IOcdm singleton object.
     *
     * @retval the instance or null on error.
     */
    virtual std::shared_ptr<IOcdm> getOcdm() const = 0;
};

class IOcdm
{
public:
    IOcdm() = default;
    virtual ~IOcdm() = default;

    IOcdm(const IOcdm &) = delete;
    IOcdm &operator=(const IOcdm &) = delete;
    IOcdm(IOcdm &&) = delete;
    IOcdm &operator=(IOcdm &&) = delete;

    /**
     * @brief Is the drm system supported.
     *
     * @param[in] keySystem : The key system to check.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus isTypeSupported(const std::string &keySystem) = 0;
};

}; // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_OCDM_H_
