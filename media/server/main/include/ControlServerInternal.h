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

#ifndef FIREBOLT_RIALTO_SERVER_CONTROL_SERVER_INTERNAL_H_
#define FIREBOLT_RIALTO_SERVER_CONTROL_SERVER_INTERNAL_H_

#include "IControlServerInternal.h"
#include "IMainThread.h"
#include <memory>

namespace firebolt::rialto::server
{
class ControlServerInternalFactory : public IControlServerInternalFactory
{
public:
    ControlServerInternalFactory() = default;
    ~ControlServerInternalFactory() override = default;

    std::shared_ptr<IControl> createControl() const override;
    std::shared_ptr<IControlServerInternal>
    createControlServerInternal(const std::shared_ptr<IControlClientServerInternal> &client) const override;
};

class ControlServerInternal : public IControlServerInternal
{
public:
    explicit ControlServerInternal(const std::shared_ptr<IMainThreadFactory> &mainThreadFactory,
                                   const std::shared_ptr<IControlClientServerInternal> &client);
    ~ControlServerInternal() override;

    void ack(uint32_t id) override;
    void setApplicationState(const ApplicationState &state) override;
    bool registerClient(std::weak_ptr<IControlClient> client, ApplicationState &appState) override;

private:
    std::shared_ptr<IControlClientServerInternal> m_client;
    std::shared_ptr<IMainThread> m_mainThread;
    uint32_t m_mainThreadClientId;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_CONTROL_SERVER_INTERNAL_H_
