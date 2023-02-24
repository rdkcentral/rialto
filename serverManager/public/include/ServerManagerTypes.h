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

#ifndef RIALTO_SERVERMANAGER_SERVICE_SERVER_MANAGER_TYPES_H_
#define RIALTO_SERVERMANAGER_SERVICE_SERVER_MANAGER_TYPES_H_

#include <stdint.h>
#include <string>

namespace rialto::servermanager::service
{
/**
 * @brief Represents all possible states of session server.
 *
 * Session Servers can be in one of four states, Not Running (session server not loaded or running),
 * Active (session server loaded, running and able to stream AV, Inactive (session server loaded but unable
 * to stream AV) and Uninitialized (session server loaded, waiting for initialization data). Error is used,
 * when something went wrong (for example connection failed)
 *
 */
enum class SessionServerState
{
    UNINITIALIZED,
    INACTIVE,
    ACTIVE,
    NOT_RUNNING,
    ERROR
};

/**
 * @brief The max resource capabilities of the platform.
 */
struct MaxResourceCapabilitites
{
    int maxPlaybacks;
    int maxWebAudioPlayers;
};

/**
 * @brief Configuration data for an application
 */
struct AppConfig
{
    std::string clientIpcSocketName; /**< Socket name that Rialto client should connect to */
    /*
     * @note Socket name can take the following forms:
     *    - Empty string, in which case Rialto server will automatically allocate the socket name, e.g. "/tmp/rialto-12"
     *    - Full path, such as "/foo/bar", in which case Rialto will use this name for the socket
     *    - Socket name, such as "bar", in which case Rialto will create the named socket in the default dir, e.g.
     * "/tmp/bar" In all cases the name can be retrieved with getAppConnectionInfo()
     */
};

} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_SERVER_MANAGER_TYPES_H_
