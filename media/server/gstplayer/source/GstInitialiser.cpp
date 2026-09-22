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
#include "IGlibWrapper.h"
#include "RialtoServerLogging.h"
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace firebolt::rialto::server
{
GstInitialiser::~GstInitialiser()
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }

#ifdef FREE_MEM_BEFORE_EXIT
    if (m_gstWrapper)
    {
        m_gstWrapper->gstDeinit();
        m_gstWrapper.reset();
    }
#endif
}

IGstInitialiser &IGstInitialiser::instance()
{
    static GstInitialiser initialiser;
    return initialiser;
}

void GstInitialiser::initialise(int *argc, char ***argv)
{
    RIALTO_SERVER_LOG_MIL("(fz-dbg)tid:%ld inside GstInitialiser.cpp initialise method about to create a thread \n", syscall(SYS_gettid));
    if (m_thread.joinable())
    {
        RIALTO_SERVER_LOG_WARN("Gstreamer is already initialised, no need to do it twice...");
        return;
    }

    m_thread = std::thread(
        [=]()
        {
            RIALTO_SERVER_LOG_MIL("(fz-dbg)tid:%ld inside GstInitialiser.cpp thread getFactory called\n", syscall(SYS_gettid));
            std::shared_ptr<firebolt::rialto::wrappers::IGstWrapperFactory> factory =
                firebolt::rialto::wrappers::IGstWrapperFactory::getFactory();
            RIALTO_SERVER_LOG_MIL("(fz-dbg)tid:%ld inside GstInitialiser.cpp getGstWrapper called\n", syscall(SYS_gettid));
            m_gstWrapper = factory->getGstWrapper();
            RIALTO_SERVER_LOG_MIL("(fz-dbg)tid:%ld inside GstInitialiser.cpp after getGstWrapper\n", syscall(SYS_gettid));
            if (!m_gstWrapper)
            {
                RIALTO_SERVER_LOG_ERROR("Failed to create the gst wrapper");
                return;
            }
            RIALTO_SERVER_LOG_MIL("(fz-dbg)tid:%ld inside GstInitialiser.cpp before gstInit\n", syscall(SYS_gettid));
            m_gstWrapper->gstInit(argc, argv);
            RIALTO_SERVER_LOG_MIL("(fz-dbg)tid:%ld inside GstInitialiser.cpp after gstInit\n", syscall(SYS_gettid));

            auto glibWrapper = firebolt::rialto::wrappers::IGlibWrapperFactory::getFactory()->getGlibWrapper();
            if (!glibWrapper)
            {
                RIALTO_SERVER_LOG_ERROR("Failed to create the glib wrapper");
                return;
            }
            glibWrapper->gThreadPoolSetMaxUnusedThreads(2);
            glibWrapper->gThreadPoolSetMaxIdleTime(5 * 1000); // 5 seconds

            // remove rialto sinks from the registry
            GstPlugin *rialtoPlugin =
                m_gstWrapper->gstRegistryFindPlugin(m_gstWrapper->gstRegistryGet(), "rialtosinks");
            if (rialtoPlugin)
            {
                m_gstWrapper->gstRegistryRemovePlugin(m_gstWrapper->gstRegistryGet(), rialtoPlugin);
                m_gstWrapper->gstObjectUnref(rialtoPlugin);
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
