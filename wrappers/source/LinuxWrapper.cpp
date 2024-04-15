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

#include "LinuxWrapper.h"
#include <sys/stat.h>

namespace firebolt::rialto::wrappers
{
std::shared_ptr<ILinuxWrapper> LinuxWrapperFactory::createLinuxWrapper() const
{
    return std::make_shared<LinuxWrapper>();
}

int LinuxWrapper::close(int fd) const
{
    return ::close(fd);
}

int LinuxWrapper::kill(int pid, int sig) const
{
    return ::kill(pid, sig);
}

int LinuxWrapper::socketpair(int domain, int type, int protocol, int sv[2]) const
{
    return ::socketpair(domain, type, protocol, sv);
}

bool LinuxWrapper::vfork(const std::function<bool(pid_t)> &function) const
{
    if (!function)
    {
        return false;
    }
    pid_t childPid{::vfork()};
    return function(childPid);
}

int LinuxWrapper::dup(int oldfd) const
{
    return ::dup(oldfd);
}

int LinuxWrapper::dup2(int oldfd, int newfd) const
{
    return ::dup2(oldfd, newfd);
}

int LinuxWrapper::open(const char *pathname, int flags, mode_t mode) const
{
    return ::open(pathname, flags, mode);
}

void LinuxWrapper::exit(int status) const
{
    _exit(status);
}

int LinuxWrapper::execve(const char *pathname, char *const argv[], char *const envp[]) const
{
    return ::execve(pathname, argv, envp);
}

pid_t LinuxWrapper::waitpid(pid_t pid, int *wstatus, int options) const
{
    return ::waitpid(pid, wstatus, options);
}

pid_t LinuxWrapper::getpid() const
{
    return ::getpid();
}

int LinuxWrapper::getpwnam_r( // NOLINT(build/function_format)
    const char *name, passwd *pwd, char *buf, size_t buflen, passwd **result) const
{
    return ::getpwnam_r(name, pwd, buf, buflen, result);
}

int LinuxWrapper::getgrnam_r( // NOLINT(build/function_format)
    const char *name, group *grp, char *buf, size_t buflen, group **result) const
{
    return ::getgrnam_r(name, grp, buf, buflen, result);
}

int LinuxWrapper::chmod(const char *pathname, mode_t mode) const
{
    return ::chmod(pathname, mode);
}

int LinuxWrapper::chown(const char *pathname, uid_t owner, gid_t group) const
{
    return ::chown(pathname, owner, group);
}
} // namespace firebolt::rialto::wrappers
