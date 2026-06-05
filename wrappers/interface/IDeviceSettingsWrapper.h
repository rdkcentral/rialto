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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_DEVICE_SETTINGS_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_DEVICE_SETTINGS_WRAPPER_H_

#include <memory>

namespace firebolt::rialto::wrappers
{
class IDeviceSettingsWrapper;

/**
 * @brief IDeviceSettingsWrapper factory class, for the IDeviceSettingsWrapper singleton object.
 */
class IDeviceSettingsWrapperFactory
{
public:
    IDeviceSettingsWrapperFactory() = default;
    virtual ~IDeviceSettingsWrapperFactory() = default;

    /**
     * @brief Gets the IDeviceSettingsWrapperFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IDeviceSettingsWrapperFactory> getFactory();

    /**
     * @brief Gets a IDeviceSettingsWrapper singleton object.
     *
     * @retval the wrapper instance or null on error.
     */
    virtual std::shared_ptr<IDeviceSettingsWrapper> getDeviceSettingsWrapper() = 0;
};

/**
 * @brief IDeviceSettingsWrapper class, it wraps the IARM interface to get the device state information
 */
class IDeviceSettingsWrapper
{
public:
    IDeviceSettingsWrapper() = default;
    virtual ~IDeviceSettingsWrapper() = default;

    /**
     * @brief Gets the current state of the HDMI connection.
     *
     * @retval true if HDMI is connected, false otherwise.
     */
    virtual bool isHdmiConnected() const = 0;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_DEVICE_SETTINGS_WRAPPER_H_
