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

#include "GstLogForwarding.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
bool gstInitalise(int argc, char **argv)
{
    std::shared_ptr<firebolt::rialto::wrappers::IGstWrapperFactory> factory =
        firebolt::rialto::wrappers::IGstWrapperFactory::getFactory();
    std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper = factory->getGstWrapper();

    if (!gstWrapper)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gst wrapper");
        return false;
    }

    gstWrapper->gstInit(&argc, &argv);

    // remove rialto sinks from the registry
    GstPlugin *rialtoPlugin = gstWrapper->gstRegistryFindPlugin(gstWrapper->gstRegistryGet(), "rialtosinks");
    if (rialtoPlugin)
    {
        gstWrapper->gstRegistryRemovePlugin(gstWrapper->gstRegistryGet(), rialtoPlugin);
        gstWrapper->gstObjectUnref(rialtoPlugin);
    }

    enableGstLogForwarding();

    return true;
}

bool gstDeinitalise()
{
    using firebolt::rialto::wrappers::IGstWrapperFactory;
    using firebolt::rialto::wrappers::IGstWrapper;
    std::shared_ptr<IGstWrapperFactory> factory = IGstWrapperFactory::getFactory();
    std::shared_ptr<IGstWrapper> gstWrapper = factory->getGstWrapper();

    if (!gstWrapper)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gst wrapper");
        return false;
    }

    gstWrapper->gstDeinit();

    return true;
}
}; // namespace firebolt::rialto::server
