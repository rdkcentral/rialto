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

#ifndef FIREBOLT_RIALTO_WRAPPERS_DEVICE_SETTINGS_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_DEVICE_SETTINGS_WRAPPER_H_

#include "IDeviceSettingsWrapper.h"
#include "host.hpp"
#include <atomic>

namespace firebolt::rialto::wrappers
{
class DeviceSettingsWrapperFactory : public IDeviceSettingsWrapperFactory
{
public:
    DeviceSettingsWrapperFactory() = default;
    ~DeviceSettingsWrapperFactory() override = default;

    std::shared_ptr<IDeviceSettingsWrapper> getDeviceSettingsWrapper() override;
};

class DeviceSettingsWrapper : public IDeviceSettingsWrapper, public device::Host::IDisplayDeviceEvents
{
public:
    DeviceSettingsWrapper();
    ~DeviceSettingsWrapper() override;

    // IDeviceSettingsWrapper methods
    bool isHdmiConnected() const override;

    // IDisplayDeviceEvents
    void OnDisplayHDMIHotPlug(dsDisplayEvent_t displayEvent) override;

private:
    template <typename T> T *baseInterface()
    {
        static_assert(std::is_base_of<T, DeviceSettingsWrapper>::value, "base type mismatch");
        return static_cast<T *>(this);
    }

private:
    std::atomic<bool> m_hdmiConnected{true};
};

} // namespace firebolt::rialto::wrappers
#endif // FIREBOLT_RIALTO_WRAPPERS_DEVICE_SETTINGS_WRAPPER_H_
