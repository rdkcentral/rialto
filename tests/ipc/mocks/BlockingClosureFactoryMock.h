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

#ifndef FIREBOLT_RIALTO_IPC_MOCK_BLOCKING_CLOSURE_FACTORY_MOCK_H_
#define FIREBOLT_RIALTO_IPC_MOCK_BLOCKING_CLOSURE_FACTORY_MOCK_H_

#include "IBlockingClosure.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::ipc::mock
{
class BlockingClosureFactoryMock : public IBlockingClosureFactory
{
public:
    BlockingClosureFactoryMock() = default;
    virtual ~BlockingClosureFactoryMock() = default;

    MOCK_METHOD(std::shared_ptr<IBlockingClosure>, createBlockingClosurePoll,
                (std::shared_ptr<::firebolt::rialto::ipc::IChannel> ipcChannel), (override));
    MOCK_METHOD(std::shared_ptr<IBlockingClosure>, createBlockingClosureSemaphore, (), (override));
};
} // namespace firebolt::rialto::ipc::mock

#endif // FIREBOLT_RIALTO_IPC_MOCK_BLOCKING_CLOSURE_FACTORY_MOCK_H_
