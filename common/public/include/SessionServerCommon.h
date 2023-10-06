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

#ifndef FIREBOLT_RIALTO_COMMON_SESSION_SERVER_COMMON_H_
#define FIREBOLT_RIALTO_COMMON_SESSION_SERVER_COMMON_H_

#include <chrono>
#include <list>
#include <stdint.h>
#include <string>

namespace firebolt::rialto::common
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
    std::string clientDisplayName; /**< Socket name that Rialto client should connect to */
};

/**
 * @brief Available permissions
 */
constexpr unsigned int kRead{4};
constexpr unsigned int kWrite{2};
constexpr unsigned int kExecute{1};

/**
 * @brief Socket permissions
 */
struct SocketPermissions
{
    unsigned int ownerPermissions{kRead | kWrite};
    unsigned int groupPermissions{kRead | kWrite};
    unsigned int otherPermissions{kRead | kWrite};
};

/**
 * @brief Configuration data for server manager
 */
struct ServerManagerConfig
{
    std::list<std::string> sessionServerEnvVars{};          /* List of environment variables, that need to be passed to
                                                               RialtoSessionServer */
    unsigned numOfPreloadedServers{0};                      /* Number of preloaded servers */
    std::string sessionServerPath{"/usr/bin/RialtoServer"}; /* Location of Rialto Session Server binary */
    std::chrono::milliseconds sessionServerStartupTimeout{
        0};                                      /* Custom session server startup timeout. If 0 - timeout disabled. */
    std::chrono::seconds healthcheckInterval{5}; /* Defines how often healthcheck messages will be sent */
    SocketPermissions sessionManagementSocketPermissions{}; /* Defines permissions of session management socket */
    unsigned numOfFailedPingsBeforeRecovery{
        3}; /* Defines how many pings have to fail before recovery action will be taken */
};

} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_SESSION_SERVER_COMMON_H_
