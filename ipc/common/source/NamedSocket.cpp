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

#include "NamedSocket.h"
#include "IpcLogging.h"
#include <grp.h>
#include <pwd.h>
#include <stdexcept>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace
{
constexpr uid_t kNoOwnerChange = -1; // -1 means chown() won't change the owner
constexpr gid_t kNoGroupChange = -1; // -1 means chown() won't change the group
} // namespace

namespace firebolt::rialto::ipc
{
INamedSocketFactory &INamedSocketFactory::getFactory()
{
    static NamedSocketFactory factory;
    return factory;
}

std::unique_ptr<INamedSocket> NamedSocketFactory::createNamedSocket(const std::string &socketPath) const
try
{
    return std::make_unique<NamedSocket>(socketPath);
}
catch (const std::runtime_error &error)
{
    RIALTO_IPC_LOG_ERROR("Failed to create named socket: %s", error.what());
    return nullptr;
}

NamedSocket::NamedSocket(const std::string &socketPath)
{
    m_sockPath = socketPath;

    // Create the socket
    m_sockFd = ::socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (m_sockFd == -1)
    {
        RIALTO_IPC_LOG_SYS_ERROR(errno, "socket error");
        throw std::runtime_error("socket error");
    }

    // get the socket lock
    if (!getSocketLock())
    {
        closeListeningSocket();
        throw std::runtime_error("lock error");
    }

    // bind to the given path
    struct sockaddr_un addr = {0};
    memset(&addr, 0x00, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(m_sockFd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        RIALTO_IPC_LOG_SYS_ERROR(errno, "bind error");

        closeListeningSocket();
        throw std::runtime_error("bind error");
    }

    // put in listening mode
    if (listen(m_sockFd, 1) == -1)
    {
        RIALTO_IPC_LOG_SYS_ERROR(errno, "listen error");

        closeListeningSocket();
        throw std::runtime_error("listen error");
    }
}

NamedSocket::~NamedSocket()
{
    closeListeningSocket();
}

int NamedSocket::getFd() const
{
    return m_sockFd;
}

bool NamedSocket::setSocketPermissions(unsigned int socketPermissions) const
{
    errno = 0;
    if (chmod(m_sockPath.c_str(), socketPermissions) != 0)
    {
        RIALTO_IPC_LOG_SYS_WARN(errno, "Failed to change the permissions on the IPC socket");
        return false;
    }
    return true;
}

bool NamedSocket::setSocketOwnership(const std::string &socketOwner, const std::string &socketGroup) const
{
    uid_t ownerId = getSocketOwnerId(socketOwner);
    gid_t groupId = getSocketGroupId(socketGroup);

    if (ownerId != kNoOwnerChange || groupId != kNoGroupChange)
    {
        errno = 0;
        if (chown(m_sockPath.c_str(), ownerId, groupId) != 0)
        {
            RIALTO_IPC_LOG_SYS_WARN(errno, "Failed to change the owner/group for the IPC socket");
        }
    }
    return true;
}

void NamedSocket::closeListeningSocket()
{
    if (!m_sockPath.empty() && (unlink(m_sockPath.c_str()) != 0) && (errno != ENOENT))
        RIALTO_IPC_LOG_SYS_ERROR(errno, "failed to remove socket @ '%s'", m_sockPath.c_str());
    if ((m_sockFd >= 0) && (close(m_sockFd) != 0))
        RIALTO_IPC_LOG_SYS_ERROR(errno, "failed to close listening socket");

    if (!m_lockPath.empty() && (unlink(m_lockPath.c_str()) != 0) && (errno != ENOENT))
        RIALTO_IPC_LOG_SYS_ERROR(errno, "failed to remove socket lock file @ '%s'", m_lockPath.c_str());
    if ((m_lockFd >= 0) && (close(m_lockFd) != 0))
        RIALTO_IPC_LOG_SYS_ERROR(errno, "failed to close socket lock file");

    m_sockFd = -1;
    m_sockPath.clear();

    m_lockFd = -1;
    m_lockPath.clear();
}

bool NamedSocket::getSocketLock()
{
    std::string lockPath = m_sockPath + ".lock";
    int fd = open(lockPath.c_str(), O_CREAT | O_CLOEXEC, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));
    if (fd < 0)
    {
        RIALTO_IPC_LOG_SYS_ERROR(errno, "failed to create / open lockfile @ '%s' (check permissions)", lockPath.c_str());
        return false;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) < 0)
    {
        RIALTO_IPC_LOG_SYS_ERROR(errno, "failed to lock lockfile @ '%s', maybe another server is running",
                                 lockPath.c_str());
        close(fd);
        return false;
    }

    struct stat sbuf = {0};
    if (stat(m_sockPath.c_str(), &sbuf) < 0)
    {
        if (errno != ENOENT)
        {
            RIALTO_IPC_LOG_SYS_ERROR(errno, "did not manage to stat existing socket @ '%s'", m_sockPath.c_str());
            close(fd);
            return false;
        }
    }
    else if ((sbuf.st_mode & S_IWUSR) || (sbuf.st_mode & S_IWGRP))
    {
        unlink(m_sockPath.c_str());
    }

    m_lockFd = fd;
    m_lockPath = std::move(lockPath);

    return true;
}

uid_t NamedSocket::getSocketOwnerId(const std::string &socketOwner) const
{
    uid_t ownerId = kNoOwnerChange;
    const size_t kBufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (!socketOwner.empty() && kBufferSize > 0)
    {
        errno = 0;
        passwd passwordStruct{};
        passwd *passwordResult = nullptr;
        char buffer[kBufferSize];
        int result = getpwnam_r(socketOwner.c_str(), &passwordStruct, buffer, kBufferSize, &passwordResult);
        if (result == 0 && passwordResult)
        {
            ownerId = passwordResult->pw_uid;
        }
        else
        {
            RIALTO_IPC_LOG_SYS_WARN(errno, "Failed to determine ownerId for '%s'", socketOwner.c_str());
        }
    }
    return ownerId;
}

gid_t NamedSocket::getSocketGroupId(const std::string &socketGroup) const
{
    gid_t groupId = kNoGroupChange;
    const size_t kBufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (!socketGroup.empty() && kBufferSize > 0)
    {
        errno = 0;
        group groupStruct{};
        group *groupResult = nullptr;
        char buffer[kBufferSize];
        int result = getgrnam_r(socketGroup.c_str(), &groupStruct, buffer, kBufferSize, &groupResult);
        if (result == 0 && groupResult)
        {
            groupId = groupResult->gr_gid;
        }
        else
        {
            RIALTO_IPC_LOG_SYS_WARN(errno, "Failed to determine groupId for '%s'", socketGroup.c_str());
        }
    }
    return groupId;
}
} // namespace firebolt::rialto::ipc