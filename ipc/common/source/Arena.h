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

#ifndef FIREBOLT_RIALTO_IPC_ARENA_H_
#define FIREBOLT_RIALTO_IPC_ARENA_H_

#include <google/protobuf/arena.h>

#include <memory>
#include <mutex>

namespace firebolt::rialto::ipc
{
class Arena
{
public:
    std::shared_ptr<google::protobuf::Arena> acquire()
    {
        std::lock_guard<std::mutex> locker(m_lock);

        // If there is no arena create a new arena
        // If current arena is still shared by others, create a new arena
        // If current arena is owned only by arena holder reset and reuse
        if (!m_arena || (m_arena.use_count() > 1))
        {
            m_arena = std::make_shared<google::protobuf::Arena>();
        }
        else
        {
            m_arena->Reset();
        }

        return m_arena;
    }

private:
    std::mutex m_lock;
    std::shared_ptr<google::protobuf::Arena> m_arena;
};
} 

#endif // FIREBOLT_RIALTO_IPC_ARENA_H_