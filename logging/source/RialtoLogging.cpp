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

#include "RialtoLogging.h"
#include "EnvVariableParser.h"
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <syslog.h>
#include <unistd.h>

/**
 * Log levels for each component. By default will print all fatals, errors, warnings & milestones.
 */
static std::atomic<RIALTO_DEBUG_LEVEL> g_rialtoLogLevels[RIALTO_COMPONENT_LAST] = {};

/**
 * Default Log levels defined by RIALTO_DEBUG environment variable
 */
static const firebolt::rialto::logging::EnvVariableParser g_envVariableParser;

/**
 * Log handler for each component. By default will use journaldLogHandler.
 */
static void consoleLogHandler(RIALTO_DEBUG_LEVEL level, const char *file, int line, const char *function,
                              const char *message, size_t messageLen);
static void journaldLogHandler(RIALTO_DEBUG_LEVEL level, const char *file, int line, const char *function,
                               const char *message, size_t messageLen);
static firebolt::rialto::logging::LogHandler g_logHandler[RIALTO_COMPONENT_LAST] = {};

/**
 * Console logging function for the library.
 */
static void consoleLogHandler(RIALTO_DEBUG_LEVEL level, const char *file, int line, const char *function,
                              const char *message, size_t messageLen)
{
    timespec ts = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    struct iovec iov[5];
    char tbuf[32];

    iov[0].iov_base = tbuf;
    iov[0].iov_len = snprintf(tbuf, sizeof(tbuf), "%.010lu.%.06lu ", ts.tv_sec, ts.tv_nsec / 1000);
    iov[0].iov_len = std::min(iov[0].iov_len, sizeof(tbuf));

    switch (level)
    {
    case RIALTO_DEBUG_LEVEL_FATAL:
        iov[1].iov_base = const_cast<void *>(reinterpret_cast<const void *>("FTL: "));
        iov[1].iov_len = 5;
        break;
    case RIALTO_DEBUG_LEVEL_ERROR:
        iov[1].iov_base = const_cast<void *>(reinterpret_cast<const void *>("ERR: "));
        iov[1].iov_len = 5;
        break;
    case RIALTO_DEBUG_LEVEL_WARNING:
        iov[1].iov_base = const_cast<void *>(reinterpret_cast<const void *>("WRN: "));
        iov[1].iov_len = 5;
        break;
    case RIALTO_DEBUG_LEVEL_MILESTONE:
        iov[1].iov_base = const_cast<void *>(reinterpret_cast<const void *>("MIL: "));
        iov[1].iov_len = 5;
        break;
    case RIALTO_DEBUG_LEVEL_INFO:
        iov[1].iov_base = const_cast<void *>(reinterpret_cast<const void *>("NFO: "));
        iov[1].iov_len = 5;
        break;
    case RIALTO_DEBUG_LEVEL_DEBUG:
        iov[1].iov_base = const_cast<void *>(reinterpret_cast<const void *>("DBG: "));
        iov[1].iov_len = 5;
        break;
    default:
        iov[1].iov_base = const_cast<void *>(reinterpret_cast<const void *>(": "));
        iov[1].iov_len = 2;
        break;
    }

    static thread_local pid_t threadId = 0;
    if (threadId <= 0)
        threadId = syscall(SYS_gettid);
    char fbuf[180];
    iov[2].iov_base = reinterpret_cast<void *>(fbuf);
    if (!file || !function || (line <= 0))
        iov[2].iov_len = snprintf(fbuf, sizeof(fbuf), "< T:%d M:? F:? L:? > ", threadId);
    else
        iov[2].iov_len =
            snprintf(fbuf, sizeof(fbuf), "< T:%d M:%.*s F:%.*s L:%d > ", threadId, 64, file, 64, function, line);
    iov[2].iov_len = std::min(iov[2].iov_len, sizeof(fbuf));
    iov[3].iov_base = const_cast<void *>(reinterpret_cast<const void *>(message));
    iov[3].iov_len = messageLen;
    iov[4].iov_base = const_cast<void *>(reinterpret_cast<const void *>("\n"));
    iov[4].iov_len = 1;
    // TODO(RIALTO-38): consider using standard write(2) and handle EINTR properly.
    std::ignore = writev(STDERR_FILENO, iov, 5);
}

/**
 * Journald logging function for the library.
 */
static void journaldLogHandler(RIALTO_DEBUG_LEVEL level, const char *file, int line, const char *function,
                               const char *message, size_t messageLen)
{
    static thread_local pid_t threadId = 0;
    if (threadId <= 0)
        threadId = syscall(SYS_gettid);
    char fbuf[180];
    if (!file || !function || (line <= 0))
    {
        snprintf(fbuf, sizeof(fbuf), "< T:%d M:? F:? L:? >", threadId);
    }
    else
    {
        snprintf(fbuf, sizeof(fbuf), "< T:%d M:%.*s F:%.*s L:%d >", threadId, 64, file, 64, function, line);
    }

    switch (level)
    {
    case RIALTO_DEBUG_LEVEL_FATAL:
        syslog(LOG_CRIT, "%s %s", fbuf, message);
        break;
    case RIALTO_DEBUG_LEVEL_ERROR:
        syslog(LOG_ERR, "%s %s", fbuf, message);
        break;
    case RIALTO_DEBUG_LEVEL_WARNING:
        syslog(LOG_WARNING, "%s %s", fbuf, message);
        break;
    case RIALTO_DEBUG_LEVEL_MILESTONE:
        syslog(LOG_NOTICE, "%s %s", fbuf, message);
        break;
    case RIALTO_DEBUG_LEVEL_INFO:
        syslog(LOG_INFO, "%s %s", fbuf, message);
        break;
    case RIALTO_DEBUG_LEVEL_DEBUG:
        syslog(LOG_DEBUG, "%s %s", fbuf, message);
        break;
    default:
        break;
    }
}

static void rialtoLog(RIALTO_COMPONENT component, RIALTO_DEBUG_LEVEL level, const char *file, const char *func,
                      int line, const char *fmt, va_list ap, const char *append)
{
    /* Must be valid component */
    if (component >= RIALTO_COMPONENT_LAST)
        return;

    /* If log levels have not been set, set to Default */
    if (!g_rialtoLogLevels[component])
        g_rialtoLogLevels[component] = g_envVariableParser.getLevel(component);

    if (!(level & g_rialtoLogLevels[component]))
        return;
    char mbuf[256];
    int len;
    len = vsnprintf(mbuf, sizeof(mbuf), fmt, ap);
    if (len < 1)
        return;
    if (len > static_cast<int>(sizeof(mbuf) - 1))
        len = sizeof(mbuf) - 1;
    if (mbuf[len - 1] == '\n')
        len--;
    mbuf[len] = '\0';
    if (append && (len < static_cast<int>(sizeof(mbuf) - 1)))
    {
        size_t extra = std::min<size_t>(strlen(append), (sizeof(mbuf) - len - 1));
        memcpy(mbuf + len, append, extra);
        len += static_cast<int>(extra);
        mbuf[len] = '\0';
    }
    const char *fname = nullptr;
    if (file)
    {
        if ((fname = strrchr(file, '/')) == nullptr)
            fname = file;
        else
            fname++;
    }

    /* If log handler had not been set, use default */
    if (g_logHandler[component])
    {
        g_logHandler[component](level, fname, line, func, mbuf, len);
    }
    else if (g_envVariableParser.isConsoleLoggingEnabled())
    {
        consoleLogHandler(level, fname, line, func, mbuf, len);
    }
    else
    {
        journaldLogHandler(level, fname, line, func, mbuf, len);
    }
}

void rialtoLogVPrintf(RIALTO_COMPONENT component, RIALTO_DEBUG_LEVEL level, const char *file, const char *func,
                      int line, const char *fmt, va_list ap)
{
    rialtoLog(component, level, file, func, line, fmt, ap, nullptr);
}

void rialtoLogPrintf(RIALTO_COMPONENT component, RIALTO_DEBUG_LEVEL level, const char *file, const char *func, int line,
                     const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rialtoLog(component, level, file, func, line, fmt, ap, nullptr);
    va_end(ap);
}

void rialtoLogSysPrintf(RIALTO_COMPONENT component, int err, RIALTO_DEBUG_LEVEL level, const char *file,
                        const char *func, int line, const char *fmt, ...)
{
    va_list ap;
    const char *errmsg{nullptr};
    char appendbuf[96];
    const char *append = nullptr;
#if defined(__linux__)
    char errbuf[64];
    errmsg = strerror_r(err, errbuf, sizeof(errbuf));
#elif defined(__APPLE__)
    char errbuf[64];
    if (strerror_r(err, errbuf, sizeof(errbuf)) != 0)
        errmsg = "Unknown error";
    else
        errmsg = errbuf;
#endif
    if (errmsg)
    {
        snprintf(appendbuf, sizeof(appendbuf), " (%d - %s)", err, errmsg);
        appendbuf[sizeof(appendbuf) - 1] = '\0';
        append = appendbuf;
    }
    va_start(ap, fmt);
    rialtoLog(component, level, file, func, line, fmt, ap, append);
    va_end(ap);
}

namespace firebolt::rialto::logging
{
RialtoLoggingStatus setLogLevels(RIALTO_COMPONENT component, RIALTO_DEBUG_LEVEL logLevels)
{
    RialtoLoggingStatus status = RIALTO_LOGGING_STATUS_ERROR;
    if (component < RIALTO_COMPONENT_LAST)
    {
        g_rialtoLogLevels[component] = logLevels;
        status = RIALTO_LOGGING_STATUS_OK;
    }

    return status;
}

RIALTO_DEBUG_LEVEL getLogLevels(RIALTO_COMPONENT component)
{
    if (component < RIALTO_COMPONENT_LAST)
    {
        if (!g_rialtoLogLevels[component])
            g_rialtoLogLevels[component] = g_envVariableParser.getLevel(component);
        return g_rialtoLogLevels[component];
    }
    return RIALTO_DEBUG_LEVEL_DEFAULT;
}

RialtoLoggingStatus setLogHandler(RIALTO_COMPONENT component, LogHandler handler)
{
    RialtoLoggingStatus status = RIALTO_LOGGING_STATUS_ERROR;
    if (component < RIALTO_COMPONENT_LAST)
    {
        /* not thread-safe, but doubt this will be an issue */
        g_logHandler[component] = std::move(handler);
        status = RIALTO_LOGGING_STATUS_OK;
    }

    return status;
}

} // namespace firebolt::rialto::logging
