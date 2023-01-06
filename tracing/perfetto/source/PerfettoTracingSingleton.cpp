//
//  PerfettoTracingSingleton.cpp
//  AppManager
//
//  Copyright © 2020 Sky UK. All rights reserved.
//
#include "PerfettoTracingSingleton.h"
#include "RialtoPerfettoTracing.h"

#if defined(RIALTO_ENABLE_TRACING)

#include <RialtoLogging.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sstream>

PerfettoTracingSingleton *PerfettoTracingSingleton::mInstance = nullptr;
pthread_rwlock_t PerfettoTracingSingleton::mInstanceLock = PTHREAD_RWLOCK_INITIALIZER;

// -----------------------------------------------------------------------------
/*!
    Get the singleton instance of the Perfetto tracing interface.

 */
PerfettoTracingSingleton* PerfettoTracingSingleton::instance()
{
    pthread_rwlock_rdlock(&mInstanceLock);

    if (!mInstance)
    {
        // need to upgrade to a write lock and then check the instance again
        // to be sure that someone else hasn't jumped in and allocated it while
        // we were unlocked.
        pthread_rwlock_unlock(&mInstanceLock);
        pthread_rwlock_wrlock(&mInstanceLock);

        if (!mInstance)
        {
            mInstance = new PerfettoTracingSingleton;
        }
    }

    pthread_rwlock_unlock(&mInstanceLock);

    return mInstance;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called at shutdown to free the singleton.

 */
void PerfettoTracingSingleton::cleanUp()
{
    pthread_rwlock_wrlock(&mInstanceLock);

    delete mInstance;

    pthread_rwlock_unlock(&mInstanceLock);
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Constructs the singleton instance.

 */
PerfettoTracingSingleton::PerfettoTracingSingleton()
    : mInitialised(false)
    , mBackends(0)
    , mTraceFileFd(-1)
{
    atexit(PerfettoTracingSingleton::cleanUp);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Flushes the trace buffer and closes any trace files.

 */
PerfettoTracingSingleton::~PerfettoTracingSingleton()
{
    if (mInProcessSession)
        stopInProcessTracing();

    if ((mTraceFileFd >= 0) && (close(mTraceFileFd) != 0))
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_TRACING, "failed to close trace file");
}

// -----------------------------------------------------------------------------
/*!
    Sets the tracing mode to either 'system' or 'in process'.

    This is a one time operation, it is not possible to change the mode once
    set.

 */
bool PerfettoTracingSingleton::initialise(unsigned int backends)
{
    if (backends == 0)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_TRACING, "at least one backend must be enabled");
        return false;
    }

    std::lock_guard<std::mutex> locker(mLock);

    if (mInitialised)
    {
        RIALTO_LOG_WARN(RIALTO_COMPONENT_TRACING, "perfetto tracing already enabled");
        return true;
    }

    perfetto::TracingInitArgs args;

    // the backends determine where trace events are recorded.  We are going to
    // use the system-wide tracing service, so that we can see our app's events
    // in context with system profiling information
    args.backends = 0;
    if (backends & RialtoPerfettoTracing::SystemBackend)
    {
        args.backends |= perfetto::kSystemBackend;
    }
    if (backends & RialtoPerfettoTracing::InProcessBackend)
    {
        args.backends |= perfetto::kInProcessBackend;
    }

    perfetto::Tracing::Initialize(args);

    // register all the track events
    perfetto::TrackEvent::Register();
#if 0
    // register the logging data source
    perfetto::DataSourceDescriptor dsd;
    dsd.set_name("com.sky.logging_data_source");
    LoggingDataSource::Register(dsd);
#endif

    // save the backends and initialised flag
    mBackends = backends;
    mInitialised = true;

    return true;
}

// -----------------------------------------------------------------------------
/*!
    Starts an in process trace, writing the trace file to the given \a fd.

 */
bool PerfettoTracingSingleton::startInProcessTracing(int fd,
                                                     const std::string &categories,
                                                     size_t maxSizeKb)
{
    std::lock_guard<std::mutex> locker(mLock);

    if (!(mBackends & RialtoPerfettoTracing::InProcessBackend))
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_TRACING, "in process tracing backend not enabled");
        return false;
    }

    if (mInProcessSession)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_TRACING, "tracing session already running");
        return false;
    }

    // dup the supplied fd because perfetto doesn't
    mTraceFileFd = fcntl(fd, F_DUPFD_CLOEXEC, 3);
    if (mTraceFileFd < 0)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_TRACING, "failed to dup trace file fd");
        return false;
    }

    // the trace config defines which types of data sources are enabled for
    // recording. We just need the "track_event" data source, which corresponds
    // to the TRACE_EVENT trace points.
    perfetto::TraceConfig config;
    config.add_buffers()->set_size_kb(1024);

    if (maxSizeKb != SIZE_MAX)
    {
        config.set_max_file_size_bytes(maxSizeKb * 1024);
    }

    // enable the standard track events
    auto *dataSourceConfig = config.add_data_sources()->mutable_config();
    dataSourceConfig->set_name("track_event");

    if (!categories.empty() && (categories != "*"))
    {
        RIALTO_LOG_INFO(RIALTO_COMPONENT_TRACING, "setting trace category filters to '%s'", categories.c_str());

        perfetto::protos::gen::TrackEventConfig trackEventConfig;
        trackEventConfig.add_disabled_categories("*");

        std::istringstream categoriesStream(categories);
        std::string category;
        while (std::getline(categoriesStream, category, ','))
        {
            trackEventConfig.add_enabled_categories(category);
        }

        dataSourceConfig->set_track_event_config_raw(trackEventConfig.SerializeAsString());
    }
#if 0
    // enable the logging data source
    auto* loggingDataSourceConfig = config.add_data_sources()->mutable_config();
    loggingDataSourceConfig->set_name("com.sky.logging_data_source");
#endif

    // start tracing
    mInProcessSession = perfetto::Tracing::NewTrace(perfetto::kInProcessBackend);
    if (!mInProcessSession)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_TRACING, "failed to create new in-process tracing session");
        return false;
    }

    mInProcessSession->Setup(config, mTraceFileFd);
    mInProcessSession->StartBlocking();

    return true;
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if currently tracing.

    For in process tracing, this will return \c true if startInProcessTracing()
    was called.  For system tracing this will only return \c true if the system
    traced daemon has started the trace.

 */
bool PerfettoTracingSingleton::isTracing() const
{
    std::lock_guard<std::mutex> locker(mLock);

    if (!mInitialised)
    {
        return false;
    }

    if (mInProcessSession)
    {
        // if mode is 'in process' then the session is only valid when a trace
        // is running
        return true;
    }

    // if in 'system' mode then we need to query the traced daemon to see
    // if our trace events are enabled
    bool started = false;
    perfetto::TrackEvent::CallIfEnabled([&](uint32_t) { started = true; });
    return started;
}

// -----------------------------------------------------------------------------
/*!
    Stops the 'in process' tracing.

 */
void PerfettoTracingSingleton::stopInProcessTracing()
{
    std::lock_guard<std::mutex> locker(mLock);

    if (!mInProcessSession)
    {
        RIALTO_LOG_WARN(RIALTO_COMPONENT_TRACING, "no 'in process' tracing session running");
        return;
    }

    // make sure everything is flushed to the target
    perfetto::TrackEvent::Flush();

    // stop tracing
    mInProcessSession->FlushBlocking(100);
    mInProcessSession->StopBlocking();
    mInProcessSession.reset();

    // close the trace file
    if ((mTraceFileFd >= 0) && (close(mTraceFileFd) != 0))
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_TRACING, "failed to close trace file");
    }

    mTraceFileFd = -1;
}

// -----------------------------------------------------------------------------
/*!
    Flushes the data from all threads out to the trace file.

 */
bool PerfettoTracingSingleton::flushInProcessTrace(const std::chrono::milliseconds &timeout)
{
    std::lock_guard<std::mutex> locker(mLock);

    if (!mInProcessSession)
    {
        RIALTO_LOG_WARN(RIALTO_COMPONENT_TRACING, "no 'in process' tracing session running");
        return false;
    }

    // make sure everything is flushed to the target
    return mInProcessSession->FlushBlocking(timeout.count());
}

#endif // defined(AI_ENABLE_TRACING)
