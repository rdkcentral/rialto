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

#include "OcdmSystem.h"
#include "OcdmCommon.h"
#include "OcdmSession.h"
#include "RialtoServerLogging.h"
#include <stdexcept>

namespace firebolt::rialto::server
{
std::shared_ptr<IOcdmSystemFactory> IOcdmSystemFactory::createFactory()
{
    std::shared_ptr<OcdmSystemFactory> factory;

    try
    {
        factory = std::make_shared<OcdmSystemFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the ocdm factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IOcdmSystem> OcdmSystemFactory::createOcdmSystem(const std::string &keySystem) const
{
    std::unique_ptr<IOcdmSystem> ocdmSystem;

    try
    {
        ocdmSystem = std::make_unique<OcdmSystem>(keySystem);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the ocdm system object, reason: %s", e.what());
    }

    return ocdmSystem;
}

OcdmSystem::OcdmSystem(const std::string &keySystem)
{
    m_systemHandle = opencdm_create_system(keySystem.c_str());
    if (!m_systemHandle)
    {
        RIALTO_SERVER_LOG_ERROR("Could not create system for %s", keySystem.c_str());
        throw std::runtime_error("Failed to create the system");
    }
}

OcdmSystem::~OcdmSystem()
{
    OpenCDMError status = opencdm_destruct_system(m_systemHandle);
    if (ERROR_NONE != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to destroy the system, reason %u", status);
    }
}

MediaKeyErrorStatus OcdmSystem::getVersion(std::string &version)
{
    char versionBuffer[64]{};
    OpenCDMError status = opencdm_system_get_version(m_systemHandle, versionBuffer);
    version = versionBuffer;

    RIALTO_SERVER_LOG_INFO("opencdm_system_get_version returned with status %u", status);

    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::getLdlSessionsLimit(uint32_t *ldlLimit)
{
    OpenCDMError status = opencdm_system_ext_get_ldl_session_limit(m_systemHandle, ldlLimit);

    RIALTO_SERVER_LOG_INFO("opencdm_system_ext_get_ldl_session_limit returned with status %u", status);

    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::deleteKeyStore()
{
    OpenCDMError status = opencdm_delete_key_store(m_systemHandle);

    RIALTO_SERVER_LOG_INFO("opencdm_delete_key_store returned with status %u", status);

    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::deleteSecureStore()
{
    OpenCDMError status = opencdm_delete_secure_store(m_systemHandle);

    RIALTO_SERVER_LOG_INFO("opencdm_delete_secure_store returned with status %u", status);

    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::getKeyStoreHash(uint8_t keyStoreHash[], uint32_t keyStoreHashLength)
{
    OpenCDMError status = opencdm_get_key_store_hash_ext(m_systemHandle, keyStoreHash, keyStoreHashLength);

    RIALTO_SERVER_LOG_INFO("opencdm_get_key_store_hash_ext returned with status %u", status);

    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::getSecureStoreHash(uint8_t secureStoreHash[], uint32_t secureStoreHashLength)
{
    OpenCDMError status = opencdm_get_secure_store_hash_ext(m_systemHandle, secureStoreHash, secureStoreHashLength);

    RIALTO_SERVER_LOG_INFO("opencdm_get_secure_store_hash_ext returned with status %u", status);

    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSystem::getDrmTime(uint64_t *time)
{
    OpenCDMError status = opencdm_system_get_drm_time(m_systemHandle, time);

    RIALTO_SERVER_LOG_INFO("opencdm_system_get_drm_time returned with status %u", status);

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
        RIALTO_SERVER_LOG_ERROR("Failed to create the ocdm session object, reason: %s", e.what());
    }

    return ocdmSession;
}

}; // namespace firebolt::rialto::server
