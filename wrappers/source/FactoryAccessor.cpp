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

std::shared_ptr<IGlibWrapperFactory> &FactoryAccessor::glibWrapperFactory()
{
#ifdef WRAPPERS_ENABLED
    if (!m_glibWrapperFactory)
    {
        m_glibWrapperFactory = std::make_shared<GlibWrapperFactory>();
    }
#endif // WRAPPERS_ENABLED
    return m_glibWrapperFactory;
}

std::shared_ptr<IGstWrapperFactory> &FactoryAccessor::gstWrapperFactory()
{
#ifdef WRAPPERS_ENABLED
    if (!m_gstWrapperFactory)
    {
        m_gstWrapperFactory = std::make_shared<GstWrapperFactory>();
    }
#endif // WRAPPERS_ENABLED
    return m_gstWrapperFactory;
}

std::shared_ptr<ILinuxWrapperFactory> &FactoryAccessor::linuxWrapperFactory()
{
#ifdef WRAPPERS_ENABLED
    if (!m_linuxWrapperFactory)
    {
        m_linuxWrapperFactory = std::make_shared<LinuxWrapperFactory>();
    }
#endif // WRAPPERS_ENABLED
    return m_linuxWrapperFactory;
}

std::shared_ptr<IOcdmFactory> &FactoryAccessor::ocdmFactory()
{
#ifdef WRAPPERS_ENABLED
    if (!m_ocdmFactory)
    {
        m_ocdmFactory = std::make_shared<OcdmFactory>();
    }
#endif // WRAPPERS_ENABLED
    return m_ocdmFactory;
}

std::shared_ptr<IOcdmSystemFactory> &FactoryAccessor::ocdmSystemFactory()
{
#ifdef WRAPPERS_ENABLED
    if (!m_ocdmSystemFactory)
    {
        m_ocdmSystemFactory = std::make_shared<OcdmSystemFactory>();
    }
#endif // WRAPPERS_ENABLED
    return m_ocdmSystemFactory;
}

std::shared_ptr<IRdkGstreamerUtilsWrapperFactory> &FactoryAccessor::rdkGstreamerUtilsWrapperFactory()
{
#ifdef WRAPPERS_ENABLED
    if (!m_rdkGstreamerUtilsWrapperFactory)
    {
        m_rdkGstreamerUtilsWrapperFactory = std::make_shared<RdkGstreamerUtilsWrapperFactory>();
    }
#endif // WRAPPERS_ENABLED
    return m_rdkGstreamerUtilsWrapperFactory;
}
} // namespace firebolt::rialto::wrappers
