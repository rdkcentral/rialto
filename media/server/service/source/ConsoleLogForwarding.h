/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_CONSOLE_LOG_FORWARDING_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_CONSOLE_LOG_FORWARDING_H_

#include <fcntl.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

namespace firebolt::rialto::server::service
{
class ConsoleLogForwarding
{
public:
    ConsoleLogForwarding();
    ~ConsoleLogForwarding();

private:
    bool start();
    void stop();

    void readLoop(int fd);

private:
    int m_stdoutPipe[2] = {-1, -1};
    int m_stderrPipe[2] = {-1, -1};

    int m_stdoutFd = -1;
    int m_stdderrFd = -1;

    std::atomic<bool> m_running{false};

    std::thread m_stdoutThread;
    std::thread m_stderrThread;
};

} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_CONSOLE_LOG_FORWARDING_H_
