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

#include <stdexcept>
#include <iostream>
#include "OcdmCommon.h"
#include "OcdmSession.h"
#include "OcdmSystem.h"
#include "opencdm/open_cdm_ext.h"

namespace firebolt::rialto::wrappers
{
std::shared_ptr<IOcdmSystem> OcdmSystemFactory::createOcdmSystem(const std::string &keySystem) const
{
    std::shared_ptr<IOcdmSystem> ocdmSystem;

    try
    {
        ocdmSystem = std::make_shared<OcdmSystem>(keySystem);
    }
    catch (const std::exception &e)
    {
    }

    return ocdmSystem;
}

OcdmSystem::OcdmSystem(const std::string &keySystem)
{
    m_systemHandle = opencdm_create_system(keySystem.c_str());
    if (!m_systemHandle)
    {
        throw std::runtime_error("Failed to create the system");
    }
}

OcdmSystem::~OcdmSystem()
{
    opencdm_destruct_system(m_systemHandle);
}

MediaKeyErrorStatus OcdmSystem::getVersion(std::string &version)
{
    char versionBuffer[64]{};
    OpenCDMError status = opencdm_system_get_version(m_systemHandle, versionBuffer);
    version = versionBuffer;
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::getLdlSessionsLimit(uint32_t *ldlLimit)
{
    OpenCDMError status = opencdm_system_ext_get_ldl_session_limit(m_systemHandle, ldlLimit);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::deleteKeyStore()
{
    OpenCDMError status = opencdm_delete_key_store(m_systemHandle);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::deleteSecureStore()
{
    OpenCDMError status = opencdm_delete_secure_store(m_systemHandle);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::getKeyStoreHash(uint8_t keyStoreHash[], uint32_t keyStoreHashLength)
{
    OpenCDMError status = opencdm_get_key_store_hash_ext(m_systemHandle, keyStoreHash, keyStoreHashLength);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::getSecureStoreHash(uint8_t secureStoreHash[], uint32_t secureStoreHashLength)
{
    OpenCDMError status = opencdm_get_secure_store_hash_ext(m_systemHandle, secureStoreHash, secureStoreHashLength);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::getDrmTime(uint64_t *time)
{
    OpenCDMError status = opencdm_system_get_drm_time(m_systemHandle, time);
    return convertOpenCdmError(status);
}

std::unique_ptr<IOcdmSession> OcdmSystem::createSession(IOcdmSessionClient *client) const
{
    std::unique_ptr<IOcdmSession> ocdmSession;

    try
    {
        ocdmSession = std::make_unique<OcdmSession>(m_systemHandle, client);
    }
    catch (const std::exception &e)
    {
    }

    return ocdmSession;
}

bool OcdmSystem::supportsServerCertificate() const
{
    return OpenCDMBool::OPENCDM_BOOL_TRUE == opencdm_system_supports_server_certificate(m_systemHandle);
}

MediaKeyErrorStatus OcdmSystem::getMetricSystemData(uint32_t *bufferLength, std::vector<uint8_t> *buffer)
{
    OpenCDMError status = opencdm_get_metric_system_data(m_systemHandle, bufferLength, buffer->data());
    return convertOpenCdmError(status);
}

}; // namespace firebolt::rialto::wrappers
