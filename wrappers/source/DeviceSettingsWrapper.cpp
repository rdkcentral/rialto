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

#include "DeviceSettingsWrapper.h"
#include "RialtoCommonLogging.h"
#include "manager.hpp"
#include <libIARM.h>
#include <libIBus.h>

namespace firebolt::rialto::wrappers
{
std::shared_ptr<IDeviceSettingsWrapper> DeviceSettingsWrapperFactory::getDeviceSettingsWrapper()
{
    // We don't want to make too many subscriptions, so let's keep a single instance of the wrapper and return it on every call
    static std::shared_ptr<IDeviceSettingsWrapper> wrapper = std::make_shared<DeviceSettingsWrapper>();
    return wrapper;
}

DeviceSettingsWrapper::DeviceSettingsWrapper()
{
    try
    {
        IARM_Result_t result;
        if (IARM_RESULT_SUCCESS == (result = IARM_Bus_Init("RIALTO")))
        {
            RIALTO_COMMON_LOG_MIL("IARM Interface Inited Successfully\n");
        }
        else
        {
            RIALTO_COMMON_LOG_MIL("IARM Interface Inited Externally : %d\n", result);
        }

        if (IARM_RESULT_SUCCESS == (result = IARM_Bus_Connect()))
        {
            RIALTO_COMMON_LOG_MIL("IARM Interface Connected in Rialto\n");
        }
        else
        {
            RIALTO_COMMON_LOG_MIL("IARM Interface Connected Externally :%d\n", result);
        }

        device::Manager::Initialize();
        device::Host::getInstance().Register(baseInterface<device::Host::IDisplayDeviceEvents>(),
                                             "Rialto::DisplaySettings");
        RIALTO_COMMON_LOG_MIL("DeviceSettingsWrapper Callback registered");
    }
    catch (...)
    {
        RIALTO_COMMON_LOG_ERROR("DeviceSettingsWrapper Callback registration failed");
    }
}

DeviceSettingsWrapper::~DeviceSettingsWrapper()
{
    try
    {
        device::Host::getInstance().UnRegister(baseInterface<device::Host::IDisplayDeviceEvents>());
        device::Manager::DeInitialize();

        RIALTO_COMMON_LOG_MIL("DeviceSettingsWrapper Callback unregistered");
    }
    catch (...)
    {
        RIALTO_COMMON_LOG_ERROR("DeviceSettingsWrapper Callback unregistration failed");
    }
}

bool DeviceSettingsWrapper::isHdmiConnected() const
{
    return m_hdmiConnected.load();
}

void DeviceSettingsWrapper::OnDisplayHDMIHotPlug(dsDisplayEvent_t displayEvent) // NOLINT(build/function_format)
{
    m_hdmiConnected.store(displayEvent == dsDISPLAY_EVENT_CONNECTED);
    RIALTO_COMMON_LOG_MIL("DeviceSettingsWrapper HDMI Callback triggered, HDMI %s",
                          m_hdmiConnected.load() ? "connected" : "disconnected");
}
} // namespace firebolt::rialto::wrappers
