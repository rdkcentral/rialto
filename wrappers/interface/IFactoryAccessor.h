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
#ifdef JSONCPP_ENABLED
#include "IJsonCppWrapperFactory.h"
#endif // JSONCPP_ENABLED
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
     * @brief Gets the IGlibWrapperFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IGlibWrapperFactory> &getGlibWrapperFactory() = 0;

    /**
     * @brief Gets the IGstWrapperFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IGstWrapperFactory> &getGstWrapperFactory() = 0;

#ifdef JSONCPP_ENABLED
    /**
     * @brief Gets the IJsonCppWrapperFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IJsonCppWrapperFactory> &getJsonCppWrapperFactory() = 0;
#endif // JSONCPP_ENABLED

    /**
     * @brief Gets the ILinuxWrapperFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<ILinuxWrapperFactory> &getLinuxWrapperFactory() = 0;

    /**
     * @brief Gets the IOcdmFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IOcdmFactory> &getOcdmFactory() = 0;

    /**
     * @brief Gets the IOcdmSystemFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IOcdmSystemFactory> &getOcdmSystemFactory() = 0;

    /**
     * @brief Gets the IRdkGstreamerUtilsWrapperFactory instance.
     *
     * @retval non-const (by purpose) reference to the factory instance ptr
     */
    virtual std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> &getRdkGstreamerUtilsWrapperFactoryFactory() = 0;

protected:
    /**
     * @brief Default constructor
     */
    IFactoryAccessor() = default;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_FACTORY_ACCESSOR_H_
