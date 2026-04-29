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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_RDK_PERF_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_RDK_PERF_WRAPPER_H_

#include <memory>

namespace firebolt::rialto::wrappers
{
class IRdkPerfWrapper;

/**
 * @brief IRdkPerfWrapperFactory factory class, returns a concrete implementation of IRdkPerfWrapper
 */
class IRdkPerfWrapperFactory
{
public:
    IRdkPerfWrapperFactory() = default;
    virtual ~IRdkPerfWrapperFactory() = default;

    /**
     * @brief Creates a IRdkPerfWrapperFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IRdkPerfWrapperFactory> createFactory();

    /**
     * @brief Creates an IRdkPerfWrapper object.
     *
     * @retval the new RDK Perf wrapper instance or null on error.
     */
    virtual std::unique_ptr<IRdkPerfWrapper> createRdkPerfWrapper(const char *szName) const = 0;
};

class IRdkPerfWrapper
{
public:
    IRdkPerfWrapper() = default;
    virtual ~IRdkPerfWrapper() = default;

    IRdkPerfWrapper(const IRdkPerfWrapper &) = delete;
    IRdkPerfWrapper &operator=(const IRdkPerfWrapper &) = delete;
    IRdkPerfWrapper(IRdkPerfWrapper &&) = delete;
    IRdkPerfWrapper &operator=(IRdkPerfWrapper &&) = delete;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_RDK_PERF_WRAPPER_H_
