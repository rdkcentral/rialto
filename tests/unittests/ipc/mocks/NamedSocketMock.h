/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "INamedSocket.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::ipc
{
class NamedSocketFactoryMock : public INamedSocketFactory
{
public:
    MOCK_METHOD(std::unique_ptr<INamedSocket>, createNamedSocket, (const std::string &socketPath), (const, override));
};

class NamedSocketMock : public INamedSocket
{
public:
    MOCK_METHOD(int, getFd, (), (const, override));
    MOCK_METHOD(bool, setSocketPermissions, (unsigned int socketPermissions), (const, override));
    MOCK_METHOD(bool, setSocketOwnership, (const std::string &socketOwner, const std::string &socketGroup),
                (const, override));
    MOCK_METHOD(bool, blockNewConnections, (), (const, override));
};
} // namespace firebolt::rialto::ipc
