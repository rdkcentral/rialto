//
//  RialtoPerfettoTracing.cpp
//  AppManager
//
//  Copyright © 2020 Sky UK. All rights reserved.
//

#include "RialtoPerfettoTracing.h"
#include "PerfettoTracingSingleton.h"

#include <RialtoLogging.h>

#include <perfetto.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#if defined(RIALTO_ENABLE_TRACING)

// reserves internal static storage for our tracing categories.
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

//PERFETTO_DEFINE_DATA_SOURCE_STATIC_MEMBERS(LoggingDataSource);


bool RialtoPerfettoTracing::initialise(unsigned backends)
{
    return PerfettoTracingSingleton::instance()->initialise(backends);
}

bool RialtoPerfettoTracing::isTracing()
{
    return PerfettoTracingSingleton::instance()->isTracing();
}

bool RialtoPerfettoTracing::startInProcessTracing(int fd,
                                            const std::string &categoryFilter,
                                            size_t maxSizeKb)
{
    return PerfettoTracingSingleton::instance()->startInProcessTracing(fd, categoryFilter, maxSizeKb);
}

bool RialtoPerfettoTracing::startInProcessTracing(const std::string &traceFile,
                                            const std::string &categoryFilter,
                                            size_t maxSizeKb)
{
    if (isTracing())
    {
        RIALTO_LOG_WARN(RIALTO_COMPONENT_TRACING, "trace already running");
        return false;
    }

    // open / create the trace file
    int fd = open(traceFile.c_str(), O_CLOEXEC | O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd < 0)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_TRACING, "failed to open / create trace file @ '%s'", traceFile.c_str());
        return false;
    }

    // start tracing to the file
    bool result = PerfettoTracingSingleton::instance()->startInProcessTracing(fd, categoryFilter, maxSizeKb);
    if (!result)
    {
        // delete the file if we failed to start the trace
        unlink(traceFile.c_str());
    }

    // close the trace file, as the instance will dup it if tracing started
    if (close(fd) != 0)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_TRACING, "failed to close trace file");
    }

    return result;
}

void RialtoPerfettoTracing::stopInProcessTracing()
{
    PerfettoTracingSingleton::instance()->stopInProcessTracing();
}

bool RialtoPerfettoTracing::flushInProcessTrace(const std::chrono::milliseconds &timeout)
{
    return PerfettoTracingSingleton::instance()->flushInProcessTrace(timeout);
}

#else  // !defined(RIALTO_ENABLE_TRACING)

bool RialtoPerfettoTracing::initialise(unsigned backends)
{
    (void) backends;

    return false;
}

bool RialtoPerfettoTracing::isTracing()
{
    return false;
}

bool RialtoPerfettoTracing::startInProcessTracing(int fd,
                                            const std::string &categoryFilter,
                                            size_t maxSizeKb)
{
    (void) fd;
    (void) categoryFilter;
    (void) maxSizeKb;

    return false;
}

bool RialtoPerfettoTracing::startInProcessTracing(const std::string &traceFile,
                                            const std::string &categoryFilter,
                                            size_t maxSizeKb)
{
    (void) traceFile;
    (void) categoryFilter;
    (void) maxSizeKb;

    return false;
}

void RialtoPerfettoTracing::stopInProcessTracing()
{
}

bool RialtoPerfettoTracing::flushInProcessTrace(const std::chrono::milliseconds &timeout)
{
    (void) timeout;

    return false;
}
#endif // !defined(RIALTO_ENABLE_TRACING)
