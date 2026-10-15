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

#include "ConsoleLogForwarding.h"
#include "RialtoLogging.h"

namespace firebolt::rialto::server::service
{
ConsoleLogForwarding::ConsoleLogForwarding()
{
    if (logging::isConsoleLoggingEnabled())
    {
        // Console logging is enabled, no need to forward logs.
        return;
    }
    start();
}

ConsoleLogForwarding::~ConsoleLogForwarding()
{
    stop();
}

bool ConsoleLogForwarding::start()
{
    if (pipe(m_stdoutPipe) == -1)
        return false;

    if (pipe(m_stderrPipe) == -1)
    {
        close(m_stdoutPipe[0]);
        close(m_stdoutPipe[1]);
        return false;
    }

    m_stdoutFd = dup(STDOUT_FILENO);
    m_stdderrFd = dup(STDERR_FILENO);

    if (m_stdoutFd == -1 || m_stdderrFd == -1)
    {
        stop();
        return false;
    }

    if (dup2(m_stdoutPipe[1], STDOUT_FILENO) == -1)
        return false;

    if (dup2(m_stderrPipe[1], STDERR_FILENO) == -1)
        return false;

    m_running = true;
    m_stdoutThread = std::thread(&ConsoleLogForwarding::readLoop, this, m_stdoutPipe[0]);

    m_stderrThread = std::thread(&ConsoleLogForwarding::readLoop, this, m_stderrPipe[0]);

    return true;
}

void ConsoleLogForwarding::stop()
{
    if (!m_running)
        return;

    m_running = false;

    if (m_stdoutFd != -1)
    {
        dup2(m_stdoutFd, STDOUT_FILENO);
        close(m_stdoutFd);
        m_stdoutFd = -1;
    }

    if (m_stdderrFd != -1)
    {
        dup2(m_stdderrFd, STDERR_FILENO);
        close(m_stdderrFd);
        m_stdderrFd = -1;
    }

    if (m_stdoutPipe[1] != -1)
    {
        close(m_stdoutPipe[1]);
        m_stdoutPipe[1] = -1;
    }

    if (m_stderrPipe[1] != -1)
    {
        close(m_stderrPipe[1]);
        m_stderrPipe[1] = -1;
    }

    if (m_stdoutThread.joinable())
        m_stdoutThread.join();

    if (m_stderrThread.joinable())
        m_stderrThread.join();

    if (m_stdoutPipe[0] != -1)
    {
        close(m_stdoutPipe[0]);
        m_stdoutPipe[0] = -1;
    }

    if (m_stderrPipe[0] != -1)
    {
        close(m_stderrPipe[0]);
        m_stderrPipe[0] = -1;
    }
}

void ConsoleLogForwarding::readLoop(int fd)
{
    std::string buffer;
    char temp[4096];

    while (m_running)
    {
        ssize_t n = read(fd, temp, sizeof(temp));

        if (n == 0)
        {
            break;
        }

        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }

        buffer.append(temp, n);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            RIALTO_LOG_EXTERNAL("%s", line.c_str());
        }
    }

    // Flush a final partial line.
    if (!buffer.empty())
    {
        RIALTO_LOG_EXTERNAL("%s", buffer.c_str());
    }
}
} // namespace firebolt::rialto::server::service
