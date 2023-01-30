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

#include <stdint.h>

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

} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_SESSION_SERVER_COMMON_H_
