/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "GstInitialiser.h"
#include "GstLogForwarding.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
GstInitialiser::~GstInitialiser()
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

IGstInitialiser &IGstInitialiser::instance()
{
    static GstInitialiser initialiser;
    return initialiser;
}

void GstInitialiser::initialise(int *argc, char ***argv)
{
    if (m_thread.joinable())
    {
        RIALTO_SERVER_LOG_WARN("Gstreamer is already initialised, no need to do it twice...");
        return;
    }

    m_thread = std::thread(
        [=]()
        {
            std::shared_ptr<firebolt::rialto::wrappers::IGstWrapperFactory> factory =
                firebolt::rialto::wrappers::IGstWrapperFactory::getFactory();
            std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper = factory->getGstWrapper();

            if (!gstWrapper)
            {
                RIALTO_SERVER_LOG_ERROR("Failed to create the gst wrapper");
                return;
            }

            gstWrapper->gstInit(argc, argv);

            // remove rialto sinks from the registry
            GstPlugin *rialtoPlugin = gstWrapper->gstRegistryFindPlugin(gstWrapper->gstRegistryGet(), "rialtosinks");
            if (rialtoPlugin)
            {
                gstWrapper->gstRegistryRemovePlugin(gstWrapper->gstRegistryGet(), rialtoPlugin);
                gstWrapper->gstObjectUnref(rialtoPlugin);
            }

            enableGstLogForwarding();

            std::unique_lock lock{m_mutex};
            m_isInitialised = true;
            m_cv.notify_all();
        });
}

void GstInitialiser::waitForInitialisation() const
{
    std::unique_lock lock{m_mutex};
    m_cv.wait(lock, [this]() { return m_isInitialised; });
}
}; // namespace firebolt::rialto::server
