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

#ifndef FIREBOLT_RIALTO_WRAPPERS_FACTORY_ACCESSOR_H_
#define FIREBOLT_RIALTO_WRAPPERS_FACTORY_ACCESSOR_H_

#include "IFactoryAccessor.h"
#include <memory>

namespace firebolt::rialto::wrappers
{
class FactoryAccessor : public IFactoryAccessor
{
public:
    FactoryAccessor();
    ~FactoryAccessor() override = default;

    std::shared_ptr<IGlibWrapperFactory> &glibWrapperFactory() override;
    std::shared_ptr<IGstWrapperFactory> &gstWrapperFactory() override;
    std::shared_ptr<ILinuxWrapperFactory> &linuxWrapperFactory() override;
    std::shared_ptr<IOcdmFactory> &ocdmFactory() override;
    std::shared_ptr<IOcdmSystemFactory> &ocdmSystemFactory() override;
    std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> &rdkGstreamerUtilsWrapperFactory() override;

private:
    std::shared_ptr<IGlibWrapperFactory> m_glibWrapperFactory{nullptr};
    std::shared_ptr<IGstWrapperFactory> m_gstWrapperFactory{nullptr};
    std::shared_ptr<ILinuxWrapperFactory> m_linuxWrapperFactory{nullptr};
    std::shared_ptr<IOcdmFactory> m_ocdmFactory{nullptr};
    std::shared_ptr<IOcdmSystemFactory> m_ocdmSystemFactory{nullptr};
    std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> m_rdkGstreamerUtilsWrapperFactory{nullptr};
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_FACTORY_ACCESSOR_H_
