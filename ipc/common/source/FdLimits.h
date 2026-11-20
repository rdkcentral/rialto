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

#ifndef FIREBOLT_RIALTO_IPC_FD_LIMITS_H_
#define FIREBOLT_RIALTO_IPC_FD_LIMITS_H_

#include <cstddef>

namespace firebolt::rialto::ipc
{
/**
 * @brief Maximum number of file descriptors in one message from a client to the server.
 *
 * The server reads at most this many; senders must not send more.
 */
constexpr std::size_t kMaxFdsClientToServer{16};

/**
 * @brief Maximum number of file descriptors in one reply or event from the server to a client.
 *
 * The client reads at most this many; senders must not send more.
 */
constexpr std::size_t kMaxFdsServerToClient{32};

} // namespace firebolt::rialto::ipc

#endif // FIREBOLT_RIALTO_IPC_FD_LIMITS_H_
