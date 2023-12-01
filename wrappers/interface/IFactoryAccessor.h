/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_FACTORY_ACCESSOR_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_FACTORY_ACCESSOR_H_

#include "IGlibWrapper.h"
#include "IGstWrapper.h"
#include "ILinuxWrapper.h"
#include "IOcdm.h"
#include "IOcdmSystem.h"
#include "IRdkGstreamerUtilsWrapper.h"
#include <memory>

namespace firebolt::rialto::wrappers
{
/**
 * @brief IFactoryAccessor: Singleton, that provides the access all wrapper factories.
 *                          Class is needed for component test, to swap "real" factories with mocks.
 */
class IFactoryAccessor
{
public:
    /**
     * @brief Default virtual destructor
     */
    virtual ~IFactoryAccessor() = default;
    IFactoryAccessor(const IFactoryAccessor &) = delete;
    IFactoryAccessor &operator=(const IFactoryAccessor &) = delete;
    IFactoryAccessor(IFactoryAccessor &&) = delete;
    IFactoryAccessor &operator=(IFactoryAccessor &&) = delete;

    /**
     * @brief Get the IFactoryAccessor singleton instance.
     *
     * @retval reference to the accessor
     */
    static IFactoryAccessor &instance();

    /**
     * @brief Access the IGlibWrapperFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IGlibWrapperFactory> &glibWrapperFactory() = 0;

    /**
     * @brief Access the IGstWrapperFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IGstWrapperFactory> &gstWrapperFactory() = 0;

    /**
     * @brief Access the ILinuxWrapperFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<ILinuxWrapperFactory> &linuxWrapperFactory() = 0;

    /**
     * @brief Access the IOcdmFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IOcdmFactory> &ocdmFactory() = 0;

    /**
     * @brief Access the IOcdmSystemFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IOcdmSystemFactory> &ocdmSystemFactory() = 0;

    /**
     * @brief Access the IRdkGstreamerUtilsWrapperFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> &rdkGstreamerUtilsWrapperFactory() = 0;

protected:
    /**
     * @brief Default constructor
     */
    IFactoryAccessor() = default;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_FACTORY_ACCESSOR_H_
