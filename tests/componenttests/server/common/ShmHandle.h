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

#ifndef FIREBOLT_RIALTO_SERVER_CT_SHM_HANDLE_H_
#define FIREBOLT_RIALTO_SERVER_CT_SHM_HANDLE_H_

#include <cstdint>

namespace firebolt::rialto::server::ct
{
class ShmHandle
{
public:
    ShmHandle() = default;
    ~ShmHandle();

    void init(std::int32_t fd, std::uint32_t length);
    std::uint8_t *getShm() const;

private:
    std::int32_t m_shmFd{-1};
    std::uint32_t m_shmBufferLen{0};
    std::uint8_t *m_shmBuffer{nullptr};
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_SHM_HANDLE_H_
