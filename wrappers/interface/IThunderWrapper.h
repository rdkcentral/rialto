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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_THUNDER_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_THUNDER_WRAPPER_H_

#include <cstdint>
#include <memory>

namespace firebolt::rialto::wrappers
{
class IThunderWrapper;

class IThunderWrapperFactory
{
public:
    IThunderWrapperFactory() = default;
    virtual ~IThunderWrapperFactory() = default;

    /**
     * @brief Gets the IThunderWrapperFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IThunderWrapperFactory> getFactory();

    /**
     * @brief Gets a IThunderWrapper singleton object.
     *
     * @retval the wrapper instance or null on error.
     */
    virtual std::shared_ptr<IThunderWrapper> getThunderWrapper() = 0;
};

class IThunderWrapper
{
public:
    IThunderWrapper() = default;
    virtual ~IThunderWrapper() = default;

    IThunderWrapper(const IThunderWrapper &) = delete;
    IThunderWrapper &operator=(const IThunderWrapper &) = delete;
    IThunderWrapper(IThunderWrapper &&) = delete;
    IThunderWrapper &operator=(IThunderWrapper &&) = delete;

    /**
     * @brief Converts error code to string
     *
     * @param[in] errorCode : The error code
     *
     * @retval the error code string
     */
    virtual const char *errorToString(std::uint32_t errorCode) const = 0;

    /**
     * @brief Checks, if operation finished with ERROR_NONE code
     *
     * @param[in] errorCode : The operation error code
     *
     * @retval true if operation finished without error
     */
    virtual bool isSuccessful(std::uint32_t errorCode) const = 0;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_THUNDER_WRAPPER_H_
