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

#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include "Cache.h"
#include "rialtoipc.pb.h"

namespace firebolt::rialto::ipc
{

using FdFields = std::vector<const google::protobuf::FieldDescriptor *>;

const std::vector<const google::protobuf::FieldDescriptor *> &
Cache::getFdFields(const google::protobuf::Descriptor *descriptor)
{
    static std::unordered_map<const google::protobuf::Descriptor *, FdFields> sCache;
    static std::shared_mutex sCacheLock;

    {
        std::shared_lock<std::shared_mutex> lock{sCacheLock};
        const auto it = sCache.find(descriptor);
        if(it != sCache.end())
        {
            return it->second;
        }

    }

    std::unique_lock<std::shared_mutex> lock{sCacheLock};
    FdFields fdFields;
    const int fieldCount = descriptor->field_count();
    fdFields.reserve(fieldCount);
    for(int index=0; index<fieldCount; index++)
    {
        const auto *fieldDescriptor = descriptor->field(index);
        if (fieldDescriptor->options().HasExtension(field_is_fd) && fieldDescriptor->options().GetExtension(field_is_fd))
        {
            fdFields.emplace_back(fieldDescriptor);
        }
    }

    const auto [it, wasInserted] = sCache.try_emplace(descriptor, std::move(fdFields));
    (void)wasInserted;
    return it->second;
}

}