/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_OCDM_SYSTEM_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_OCDM_SYSTEM_H_

#include "IOcdmSession.h"
#include "IOcdmSessionClient.h"

#include <MediaCommon.h>
#include <memory>
#include <stdint.h>
#include <string>

namespace firebolt::rialto::wrappers
{
class IOcdmSystem;

/**
 * @brief IOcdmSystem factory class, for the IOcdmSystem object.
 */
class IOcdmSystemFactory
{
public:
    IOcdmSystemFactory() = default;
    virtual ~IOcdmSystemFactory() = default;

    /**
     * @brief Gets the IOcdmSystemFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IOcdmSystemFactory> createFactory();

    /**
     * @brief Creates a IOcdmSystem object.
     *
     * @param[in] keySystem : The key system to create.
     *
     * @retval the wrapper instance or null on error.
     */
    virtual std::shared_ptr<IOcdmSystem> createOcdmSystem(const std::string &keySystem) const = 0;
};

class IOcdmSystem
{
public:
    IOcdmSystem() = default;
    virtual ~IOcdmSystem() = default;

    IOcdmSystem(const IOcdmSystem &) = delete;
    IOcdmSystem &operator=(const IOcdmSystem &) = delete;
    IOcdmSystem(IOcdmSystem &&) = delete;
    IOcdmSystem &operator=(IOcdmSystem &&) = delete;

    /**
     * @brief Get the system version.
     *
     * @param[out] version : The string of the version.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus getVersion(std::string &version) = 0;

    /**
     * @brief Get the limit on the number of ldl key sessions for the object's key system
     *
     * @param[out] ldlLimit : the limit on the number of ldl key sessions.
     *
     * @retval the return status value.
     */
    virtual MediaKeyErrorStatus getLdlSessionsLimit(uint32_t *ldlLimit) = 0;

    /**
     * @brief Delete the key store for the object's key system
     *
     * @retval the return status value.
     */
    virtual MediaKeyErrorStatus deleteKeyStore() = 0;

    /**
     * @brief Delete the DRM store for the object's key system
     *
     * @retval the return status value.
     */
    virtual MediaKeyErrorStatus deleteSecureStore() = 0;

    /**
     * @brief Gets a hash of the Key store for the object's key system
     *
     * @param[out] keyStoreHash : the hash value
     *
     * @retval the return status value.
     */
    virtual MediaKeyErrorStatus getKeyStoreHash(uint8_t keyStoreHash[], uint32_t keyStoreHashLength) = 0;

    /**
     * @brief Gets a hash of the DRM store for the object's key system
     *
     * @param[out] drmStoreHash : the hash value
     *
     * @retval the return status value.
     */
    virtual MediaKeyErrorStatus getSecureStoreHash(uint8_t secureStoreHash[], uint32_t secureStoreHashLength) = 0;

    /**
     * @brief Get the DRM system time for the object's key system
     *
     * @param[out]  time : the DRM system time
     *
     * @retval the return status value.
     */
    virtual MediaKeyErrorStatus getDrmTime(uint64_t *time) = 0;

    /**
     * @brief Creates a IOcdmSession and returns a pointer to that object.
     *
     * @param[in]  client       : Client object for callbacks.
     *
     * @retval the new IOcdmSession object or null on error.
     */
    virtual std::unique_ptr<IOcdmSession> createSession(IOcdmSessionClient *client) const = 0;

    /**
     * @brief Gets support server certificate.
     *
     * Some DRMs (e.g. WideVine) use a system-wide server certificate. This method
     * gets if system has support for that certificate.
     *
     * @retval true if server certificate is supported
     */
    virtual bool supportsServerCertificate() const = 0;

    /**
     * @brief Get the metric system data
     *
     * @param[out]  buffer       : the buffer to store the data
     *
     * @retval the return status value.
     */
     virtual MediaKeyErrorStatus getMetricSystemData(uint32_t *bufferLength, std::vector<uint8_t> *buffer) = 0;
};

}; // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_OCDM_SYSTEM_H_
