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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_I_MAIN_THREAD_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_I_MAIN_THREAD_H_

#include "IMainThread.h"
#include <functional>
#include <utility>

namespace firebolt::rialto::server::service
{
/**
 * @brief The definition of the IMainThread interface.
 */
class IMainThread
{
public:
    using Task = std::function<void()>;

    IMainThread() = default;
    virtual ~IMainThread() = default;

    IMainThread(const IMainThread &) = delete;
    IMainThread(IMainThread &&) = delete;
    IMainThread &operator=(const IMainThread &) = delete;
    IMainThread &operator=(IMainThread &&) = delete;

    virtual void enqueueTask(Task task) = 0;
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_I_MAIN_THREAD_H_
