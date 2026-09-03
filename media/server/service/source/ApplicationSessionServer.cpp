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

#include "ApplicationSessionServer.h"

namespace firebolt::rialto::server
{
std::unique_ptr<IApplicationSessionServerFactory> IApplicationSessionServerFactory::getFactory()
{
    return std::make_unique<ApplicationSessionServerFactory>();
}

std::unique_ptr<IApplicationSessionServer> ApplicationSessionServerFactory::createApplicationSessionServer() const
{
    return std::make_unique<ApplicationSessionServer>();
}

ApplicationSessionServer::ApplicationSessionServer()
{
    // Safely initialize GstCapabilities with null-safety check
    auto gstFactory = IGstCapabilitiesFactory::getFactory();
    if (gstFactory)
    {
        m_gstCapabilities = gstFactory->createGstCapabilities();
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("Failed to get GstCapabilitiesFactory, GStreamer fallback path will be unavailable");
    }

    // Create orchestrator with potentially null GStreamer capabilities
    // (MediaCapabilities constructor handles null gracefully)
    m_mediaCapabilities = std::make_shared<MediaCapabilities>(m_gstCapabilities);
}

bool ApplicationSessionServer::init(int argc, char *argv[])
{
    return m_serviceManager.initialize(argc, argv);
}

void ApplicationSessionServer::startService()
{
    m_serviceManager.startService();
}
} // namespace firebolt::rialto::server
