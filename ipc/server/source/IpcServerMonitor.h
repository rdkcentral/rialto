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

#ifndef FIREBOLT_RIALTO_IPC_IPC_SERVER_MONITOR_H_
#define FIREBOLT_RIALTO_IPC_IPC_SERVER_MONITOR_H_

#include "FileDescriptor.h"
#include "IIpcServer.h"
#include "SimpleBufferPool.h"

#include "rialtoipc-transport.pb.h"

#include <list>
#include <map>
#include <memory>
#include <string>

namespace firebolt::rialto::ipc
{
class ServerMonitor
{
public:
    ServerMonitor() = default;
    ~ServerMonitor() = default;

    bool addMonitorSocket(const FileDescriptor &socket);

    void clientConnected(const std::string &socketPath, uint64_t clientId, const std::shared_ptr<const IClient> &client);
    void clientDisconnected(uint64_t clientId);

    void monitorCall(uint64_t clientId, const transport::MethodCall &call, bool noReply);
    void monitorReply(uint64_t clientId, const transport::MethodCallReply &reply);
    void monitorError(uint64_t clientId, const transport::MethodCallError &error);
    void monitorEvent(uint64_t clientId, const transport::EventFromServer &event);

private:
    static void setTimestamps(transport::MonitorMessage *message);

    bool sendToMonitor(int sock, const transport::MonitorMessage &message);
    void sendToMonitors(const transport::MonitorMessage &event);

private:
    static const size_t m_kMaxMessageSize;

    std::list<FileDescriptor> m_monitorSockets;
    std::map<uint64_t, transport::ClientDetails> m_clientDetails;

    SimpleBufferPool m_bufferPool;
};

} // namespace firebolt::rialto::ipc

#endif // FIREBOLT_RIALTO_IPC_IPC_SERVER_MONITOR_H_
