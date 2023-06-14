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

#ifndef FIREBOLT_RIALTO_SERVER_HEARTBEAT_PROCEDURE_H_
#define FIREBOLT_RIALTO_SERVER_HEARTBEAT_PROCEDURE_H_

#include "IHeartbeatProcedure.h"
#include <memory>
#include <set>

namespace firebolt::rialto::server
{
class HeartbeatProcedureFactory : public IHeartbeatProcedureFactory
{
public:
    HeartbeatProcedureFactory() = default;
    ~HeartbeatProcedureFactory() override = default;
    std::shared_ptr<IHeartbeatProcedure> createHeartbeatProcedure(const std::shared_ptr<IAckSender> &ackSender,
                                                                  std::int32_t pingId) const override;
};

class HeartbeatProcedure : public std::enable_shared_from_this<HeartbeatProcedure>, public IHeartbeatProcedure
{
    class HeartbeatHandler : public IHeartbeatHandler
    {
    public:
        HeartbeatHandler(const std::shared_ptr<HeartbeatProcedure> &procedure, int controlId, std::int32_t pingId);
        ~HeartbeatHandler() override;

        void pingSent() override;
        void error() override;
        std::int32_t id() const override;

    private:
        std::shared_ptr<HeartbeatProcedure> m_procedure;
        const int m_kControlId;
        const std::int32_t m_kPingId;
        bool m_success;
    };

public:
    HeartbeatProcedure(const std::shared_ptr<IAckSender> &ackSender, std::int32_t pingId);
    ~HeartbeatProcedure() override;
    std::unique_ptr<IHeartbeatHandler> createHandler(int controlId) override;

private:
    void onStart(int controlId);
    void onFinish(int controlId, bool success);

private:
    std::shared_ptr<IAckSender> m_ackSender;
    const std::int32_t m_kPingId;
    // Mutex not needed for attributes below - all methods are called in main thread.
    bool m_success;
    std::set<int> m_pingsSent;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_HEARTBEAT_PROCEDURE_H_
