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

#ifndef FIREBOLT_RIALTO_COMMON_I_LINUX_WRAPPER_H_
#define FIREBOLT_RIALTO_COMMON_I_LINUX_WRAPPER_H_

#include <fcntl.h>
#include <functional>
#include <memory>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace firebolt::rialto::common
{
class ILinuxWrapper;

/**
 * @brief ILinuxWrapperFactory factory class, returns a concrete implementation of ILinuxWrapper
 */
class ILinuxWrapperFactory
{
public:
    ILinuxWrapperFactory() = default;
    virtual ~ILinuxWrapperFactory() = default;

    /**
     * @brief Creates a ILinuxWrapperFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<ILinuxWrapperFactory> createFactory();

    /**
     * @brief Creates an ILinuxWrapper object.
     *
     * @retval the new linux wrapper instance or null on error.
     */
    virtual std::unique_ptr<ILinuxWrapper> createLinuxWrapper() const = 0;
};

class ILinuxWrapper
{
public:
    ILinuxWrapper() = default;
    virtual ~ILinuxWrapper() = default;

    ILinuxWrapper(const ILinuxWrapper &) = delete;
    ILinuxWrapper &operator=(const ILinuxWrapper &) = delete;
    ILinuxWrapper(ILinuxWrapper &&) = delete;
    ILinuxWrapper &operator=(ILinuxWrapper &&) = delete;

    /**
     * @brief Closes a file descriptor
     *
     * @param[in] fd : The file descriptor
     *
     * @retval 0 on success, on error, -1 is returned and errno is set to indicate the error.
     */
    virtual int close(int fd) const = 0;

    /**
     * @brief Send signal to a process
     *
     * @param[in] pid : The process id
     * @param[in] sig : The signal to be sent to the process
     *
     * @retval 0 on success, on error, -1 is returned and errno is set to indicate the error.
     */
    virtual int kill(int pid, int sig) const = 0;

    /**
     * @brief Create a pair of connected sockets
     *
     * @param[in] domain   : The socket domain
     * @param[in] type     : The socket type
     * @param[in] protocol : The optionally specified protocol
     * @param[out] sv      : The file descriptors used in referencing the new sockets
     *
     * @retval 0 on success, on error, -1 is returned and errno is set to indicate the error.
     */
    virtual int socketpair(int domain, int type, int protocol, int sv[2]) const = 0;

    /**
     * @brief Create a child process and block parent
     *
     * @param[in] function   : The function called after fork (the child must not return from wrapper function after
     *                         calling fork)
     *
     * @retval value returned by function given as a parameter
     */
    virtual bool vfork(const std::function<bool(pid_t)> &function) const = 0;

    /**
     * @brief Duplicate a file descriptor
     *
     * @param[in] oldfd : The file descriptor to duplicate
     *
     * @retval Duplicated file descriptor on success. On error, -1 is returned, and errno is set to indicate the error.
     */
    virtual int dup(int oldfd) const = 0;

    /**
     * @brief Duplicate a file descriptor
     *
     * @param[in] oldfd : The file descriptor to duplicate
     * @param[in] newfd : The file descriptor to adjust
     *
     * @retval Duplicated file descriptor on success. On error, -1 is returned, and errno is set to indicate the error.
     */
    virtual int dup2(int oldfd, int newfd) const = 0;

    /**
     * @brief Open and possibly create a file
     *
     * @param[in] pathname : The file path
     * @param[in] flags    : The flags
     * @param[in] mode     : The mode
     *
     * @retval New file descriptor on success. On error, -1 is returned, and errno is set to indicate the error.
     */
    virtual int open(const char *pathname, int flags, mode_t mode) const = 0;

    /**
     * @brief Terminate the calling process
     *
     * @param[in] status : The exit status
     */
    virtual void exit(int status) const = 0;

    /**
     * @brief Execute program
     *
     * @param[in] pathname : The program path
     * @param[in] argv     : The command-line arguments
     * @param[in] envp     : The environment of the new program
     *
     * @retval Does not return on success. On error, -1 is returned, and errno is set to indicate the error.
     */
    virtual int execve(const char *pathname, char *const argv[], char *const envp[]) const = 0;

    /**
     * @brief Wait for process to change state
     *
     * @param[in]  pid     : The process id
     * @param[out] wstatus : The status information
     * @param[in]  options : The function options
     *
     * @retval on success, returns the process ID of the child whose state has changed. On error, -1 is returned.
     */
    virtual pid_t waitpid(pid_t pid, int *wstatus, int options) const = 0;

    /**
     * @brief Get process identification
     *
     * @retval on success, returns the process ID of the child whose state has changed. On error, -1 is returned.
     */
    virtual pid_t getpid() const = 0;
};
} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_I_LINUX_WRAPPER_H_
