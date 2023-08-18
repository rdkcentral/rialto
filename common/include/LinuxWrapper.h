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

#ifndef FIREBOLT_RIALTO_COMMON_LINUX_WRAPPER_H_
#define FIREBOLT_RIALTO_COMMON_LINUX_WRAPPER_H_

#include "ILinuxWrapper.h"

namespace firebolt::rialto::common
{
class LinuxWrapperFactory : public ILinuxWrapperFactory
{
public:
    LinuxWrapperFactory() = default;
    ~LinuxWrapperFactory() override = default;

    std::unique_ptr<ILinuxWrapper> createLinuxWrapper() const override;
};

class LinuxWrapper : public ILinuxWrapper
{
public:
    LinuxWrapper() = default;
    ~LinuxWrapper() override = default;

    int close(int fd) const override;
    int kill(int pid, int sig) const override;
    int socketpair(int domain, int type, int protocol, int sv[2]) const override;
    pid_t vfork() const override;
    int dup(int oldfd) const override;
    int dup2(int oldfd, int newfd) const override;
    int open(const char *pathname, int flags, mode_t mode) const override;
    void exit(int status) const override;
    int execve(const char *pathname, char *const argv[], char *const envp[]) const override;
    pid_t waitpid(pid_t pid, int *wstatus, int options) const override;
    pid_t getpid() const override;
};
} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_LINUX_WRAPPER_H_
