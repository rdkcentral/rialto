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

#include "FactoryAccessor.h"

#ifdef WRAPPERS_ENABLED
#include "GlibWrapper.h"
#include "GstWrapper.h"
#ifdef JSONCPP_ENABLED
#include "JsonCppWrapperFactory.h"
#endif // JSONCPP_ENABLED
#include "LinuxWrapper.h"
#include "Ocdm.h"
#include "OcdmSystem.h"
#include "RdkGstreamerUtilsWrapper.h"
#endif // WRAPPERS_ENABLED

namespace firebolt::rialto::wrappers
{
IFactoryAccessor &IFactoryAccessor::instance()
{
    static FactoryAccessor accessor;
    return accessor;
}

FactoryAccessor::FactoryAccessor()
{
#ifdef WRAPPERS_ENABLED
    m_glibWrapperFactory = std::make_shared<GlibWrapperFactory>();
    m_gstWrapperFactory = std::make_shared<GstWrapperFactory>();
#ifdef JSONCPP_ENABLED
    m_jsonCppWrapperFactory = std::make_shared<JsonCppWrapperFactory>();
#endif // JSONCPP_ENABLED
    m_linuxWrapperFactory = std::make_shared<LinuxWrapperFactory>();
    m_ocdmFactory = std::make_shared<OcdmFactory>();
    m_ocdmSystemFactory = std::make_shared<OcdmSystemFactory>();
    m_rdkGstreamerUtilsWrapperFactory = std::make_shared<RdkGstreamerUtilsWrapperFactory>();
#endif // WRAPPERS_ENABLED
}

std::shared_ptr<IGlibWrapperFactory> &FactoryAccessor::getGlibWrapperFactory()
{
    return m_glibWrapperFactory;
}

std::shared_ptr<IGstWrapperFactory> &FactoryAccessor::getGstWrapperFactory()
{
    return m_gstWrapperFactory;
}

#ifdef JSONCPP_ENABLED
std::shared_ptr<IJsonCppWrapperFactory> &FactoryAccessor::getJsonCppWrapperFactory()
{
    return m_jsonCppWrapperFactory;
}
#endif // JSONCPP_ENABLED

std::shared_ptr<ILinuxWrapperFactory> &FactoryAccessor::getLinuxWrapperFactory()
{
    return m_linuxWrapperFactory;
}

std::shared_ptr<IOcdmFactory> &FactoryAccessor::getOcdmFactory()
{
    return m_ocdmFactory;
}

std::shared_ptr<IOcdmSystemFactory> &FactoryAccessor::getOcdmSystemFactory()
{
    return m_ocdmSystemFactory;
}

std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> &FactoryAccessor::getRdkGstreamerUtilsWrapperFactoryFactory()
{
    return m_rdkGstreamerUtilsWrapperFactory;
}
} // namespace firebolt::rialto::wrappers
