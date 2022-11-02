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

#include "IpcServerMonitor.h"
#include "IIpcServer.h"
#include "IpcLogging.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>

#define USECS_PER_SECS uint64_t(1000000)
#define NSECS_PER_USECS uint64_t(1000)

namespace firebolt::rialto::ipc
{
const size_t ServerMonitor::m_kMaxMessageSize = (256 * 1024);

bool ServerMonitor::addMonitorSocket(const FileDescriptor &socket)
{
    // sanity check that the fd is a socket
    struct stat buf = {0};
    if (fstat(socket.fd(), &buf) < 0)
    {
        RIALTO_IPC_LOG_SYS_WARN(errno, "failed to stat supplied monitor socket");
        return false;
    }
    if (!S_ISSOCK(buf.st_mode))
    {
        RIALTO_IPC_LOG_WARN("monitor fd supplied is not a socket");
        return false;
    }

    // and check it is not a listening socket
    int accepting = 0;
    socklen_t l = sizeof(accepting);

    if (getsockopt(socket.fd(), SOL_SOCKET, SO_ACCEPTCONN, &accepting, &l) < 0)
    {
        RIALTO_IPC_LOG_SYS_WARN(errno, "failed to get monitor socket type");
        return false;
    }
    if (accepting > 0)
    {
        RIALTO_IPC_LOG_SYS_WARN(errno, "listening socket was passed as monitor socket, ignoring");
        return false;
    }

    // TODO(LLDEV-27303): check someone hasn't supplied a socket back to us, such that we could get into an
    // infinite loop of sending monitor messages back to ourselves

    // shutdown the read end of the received socket
    if (shutdown(socket.fd(), SHUT_RD) != 0)
    {
        RIALTO_IPC_LOG_SYS_WARN(errno, "listening socket was passed as monitor socket, ignoring");
        return false;
    }

    // send out details of all the currently connected sockets
    transport::MonitorMessage message;
    auto currentClients = message.mutable_current_clients();
    auto clients = currentClients->mutable_clients();
    for (const auto &entry : m_clientDetails)
        clients->insert({entry.first, entry.second});

    // send all the active client details to the monitor
    if (!sendToMonitor(socket.fd(), message))
    {
        RIALTO_IPC_LOG_WARN("failed to send all client details to the monitor socket");
        return false;
    }

    // finally, can add the socket to the internal list of monitors
    m_monitorSockets.emplace_back(socket);
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal
    \static

*/
void ServerMonitor::setTimestamps(transport::MonitorMessage *message)
{
    timespec tsReal = {0}, tsMono = {0};
    clock_gettime(CLOCK_REALTIME, &tsReal);
    clock_gettime(CLOCK_MONOTONIC, &tsMono);

    message->set_timestamp_real((uint64_t(tsReal.tv_sec) * USECS_PER_SECS) + (uint64_t(tsReal.tv_nsec) / NSECS_PER_USECS));
    message->set_timestamp_mono((uint64_t(tsMono.tv_sec) * USECS_PER_SECS) + (uint64_t(tsMono.tv_nsec) / NSECS_PER_USECS));
}

// -----------------------------------------------------------------------------
/*!
    \internal

*/
void ServerMonitor::clientConnected(const std::string &socketPath, uint64_t clientId,
                                    const std::shared_ptr<const IClient> &client)
{
    // add the client details to our internal map
    transport::ClientDetails details;
    details.set_pid(client->getClientPid());
    details.set_uid(client->getClientUserId());
    details.set_gid(client->getClientGroupId());
    details.set_socket_path(socketPath);
    m_clientDetails.emplace(clientId, std::move(details));

    // send notification to any monitors
    if (m_monitorSockets.empty())
        return;

    transport::MonitorMessage message;
    setTimestamps(&message);

    auto client_ = message.mutable_client_connected();
    client_->set_client_id(clientId);

    auto details_ = client_->mutable_details();
    details_->set_pid(client->getClientPid());
    details_->set_uid(client->getClientUserId());
    details_->set_gid(client->getClientGroupId());
    details_->set_socket_path(socketPath);

    sendToMonitors(message);
}

// -----------------------------------------------------------------------------
/*!
    \internal

*/
void ServerMonitor::clientDisconnected(uint64_t clientId)
{
    // remove from the internal map of clients
    m_clientDetails.erase(clientId);

    // send notification to any clients
    if (m_monitorSockets.empty())
        return;

    transport::MonitorMessage message;
    setTimestamps(&message);

    message.set_client_disconnected(clientId);

    sendToMonitors(message);
}

// -----------------------------------------------------------------------------
/*!
    \internal

*/
void ServerMonitor::monitorCall(uint64_t clientId, const transport::MethodCall &call, bool noReply)
{
    if (m_monitorSockets.empty())
        return;

    transport::MonitorMessage message;
    setTimestamps(&message);

    auto call_ = message.mutable_call();
    call_->set_client_id(clientId);
    if (noReply)
        call_->set_no_reply(true);
    call_->mutable_call()->CopyFrom(call);

    sendToMonitors(message);
}

// -----------------------------------------------------------------------------
/*!
    \internal

*/
void ServerMonitor::monitorReply(uint64_t clientId, const transport::MethodCallReply &reply)
{
    if (m_monitorSockets.empty())
        return;

    transport::MonitorMessage message;
    setTimestamps(&message);

    auto reply_ = message.mutable_reply();
    reply_->set_target_client_id(clientId);
    reply_->mutable_reply()->CopyFrom(reply);

    sendToMonitors(message);
}

// -----------------------------------------------------------------------------
/*!
    \internal

*/
void ServerMonitor::monitorError(uint64_t clientId, const transport::MethodCallError &error)
{
    if (m_monitorSockets.empty())
        return;

    transport::MonitorMessage message;
    setTimestamps(&message);

    auto error_ = message.mutable_error();
    error_->set_target_client_id(clientId);
    error_->mutable_error()->CopyFrom(error);

    sendToMonitors(message);
}

// -----------------------------------------------------------------------------
/*!
    \internal

*/
void ServerMonitor::monitorEvent(uint64_t clientId, const transport::EventFromServer &event)
{
    if (m_monitorSockets.empty())
        return;

    transport::MonitorMessage message;
    setTimestamps(&message);

    auto event_ = message.mutable_event();
    event_->set_target_client_id(clientId);
    event_->mutable_event()->CopyFrom(event);

    sendToMonitors(message);
}

// -----------------------------------------------------------------------------
/*!
    \internal

*/
bool ServerMonitor::sendToMonitor(int sock, const transport::MonitorMessage &message)
{
    // check the reply will fit
    const size_t messageSize = message.ByteSizeLong();
    if (messageSize > m_kMaxMessageSize)
    {
        RIALTO_IPC_LOG_ERROR("monitor message to big to fit in buffer (size %zu, max size %zu)", messageSize,
                             m_kMaxMessageSize);
        return false;
    }

    // allocate a buffer from the pool for storing the serialised message
    auto dataBuf = m_bufferPool.allocateShared<uint8_t>(messageSize);

    // serialise the reply to the message buffer
    message.SerializeWithCachedSizesToArray(dataBuf.get());

    // construct the message
    struct msghdr msg = {nullptr};
    struct iovec io = {.iov_base = dataBuf.get(), .iov_len = messageSize};

    bzero(&msg, sizeof(msg));
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = nullptr;
    msg.msg_controllen = 0;

    // send to each installed monitor socket, on any send failure we close the monitor socket
    if (TEMP_FAILURE_RETRY(sendmsg(sock, &msg, MSG_NOSIGNAL | MSG_DONTWAIT)) != static_cast<ssize_t>(io.iov_len))
    {
        RIALTO_IPC_LOG_SYS_ERROR(errno, "failed to send the complete monitor message, closing socket");
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

*/
void ServerMonitor::sendToMonitors(const transport::MonitorMessage &message)
{
    // sanity check the reply will fit
    const size_t messageSize = message.ByteSizeLong();
    if (messageSize > m_kMaxMessageSize)
    {
        RIALTO_IPC_LOG_ERROR("monitor message to big to fit in buffer (size %zu, max size %zu)", messageSize,
                             m_kMaxMessageSize);
        return;
    }

    // allocate a buffer from the pool for storing the serialised message
    uint8_t *dataBuf = m_bufferPool.allocate<uint8_t>(messageSize);

    // serialise the reply to the message buffer
    message.SerializeWithCachedSizesToArray(dataBuf);

    // construct the message
    struct msghdr msg = {nullptr};
    struct iovec io = {.iov_base = dataBuf, .iov_len = messageSize};

    bzero(&msg, sizeof(msg));
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = nullptr;
    msg.msg_controllen = 0;

    // send to each installed monitor socket, on any send failure we close the monitor socket
    auto it = m_monitorSockets.begin();
    while (it != m_monitorSockets.end())
    {
        if (TEMP_FAILURE_RETRY(sendmsg(it->fd(), &msg, MSG_NOSIGNAL | MSG_DONTWAIT)) != static_cast<ssize_t>(io.iov_len))
        {
            if (errno == EPIPE)
                RIALTO_IPC_LOG_INFO("monitor socket closed by remote end");
            else if (errno == EWOULDBLOCK)
                RIALTO_IPC_LOG_WARN("monitor socket is blocked, failed to add message, also closing socket");
            else
                RIALTO_IPC_LOG_SYS_ERROR(errno, "failed to send the complete monitor message, closing socket");

            it = m_monitorSockets.erase(it);
        }
        else
        {
            ++it;
        }
    }

    m_bufferPool.deallocate(dataBuf);
}

} // namespace firebolt::rialto::ipc
