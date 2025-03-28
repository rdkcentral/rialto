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

#ifndef FIREBOLT_RIALTO_WRAPPERS_OCDM_SYSTEM_H_
#define FIREBOLT_RIALTO_WRAPPERS_OCDM_SYSTEM_H_

#include "IOcdmSystem.h"
#include "opencdm/open_cdm.h"
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::wrappers
{
/**
 * @brief IOcdmSystem factory class definition.
 */
class OcdmSystemFactory : public IOcdmSystemFactory
{
public:
    std::shared_ptr<IOcdmSystem> createOcdmSystem(const std::string &keySystem) const override;
};

/**
 * @brief The definition of the OcdmSystem.
 */
class OcdmSystem : public IOcdmSystem
{
public:
    /**
     * @brief The constructor.
     */
    explicit OcdmSystem(const std::string &keySystem);

    /**
     * @brief Virtual destructor.
     */
    virtual ~OcdmSystem();

    MediaKeyErrorStatus getVersion(std::string &version) override;
    MediaKeyErrorStatus getLdlSessionsLimit(uint32_t *ldlLimit) override;
    MediaKeyErrorStatus deleteKeyStore() override;
    MediaKeyErrorStatus deleteSecureStore() override;
    MediaKeyErrorStatus getKeyStoreHash(uint8_t keyStoreHash[], uint32_t keyStoreHashLength) override;
    MediaKeyErrorStatus getSecureStoreHash(uint8_t secureStoreHash[], uint32_t secureStoreHashLength) override;
    MediaKeyErrorStatus getDrmTime(uint64_t *time) override;
    MediaKeyErrorStatus getMetricSystemData(uint32_t &bufferLength, std::vector<uint8_t> &buffer, int &error) override;
    std::unique_ptr<IOcdmSession> createSession(IOcdmSessionClient *client) const override;
    bool supportsServerCertificate() const override;

private:
    /**
     * @brief The system handle.
     */
    struct OpenCDMSystem *m_systemHandle;
};

}; // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_OCDM_SYSTEM_H_
