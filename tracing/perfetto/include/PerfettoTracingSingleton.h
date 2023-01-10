//
//  PerfettoTracingSingleton.h
//
//  Copyright © 2023 Sky UK. All rights reserved.
//
#ifndef PERFETTOTRACINGSINGLETON_H
#define PERFETTOTRACINGSINGLETON_H

#include <perfetto.h>

#include <pthread.h>

#include <string>
#include <memory>
#include <mutex>
#include <chrono>

// define an empty set of categories if tracing is disabled
#if !defined(RIALTO_ENABLE_TRACING)
    PERFETTO_DEFINE_CATEGORIES();
#endif


class PerfettoTracingSingleton
{
private:
    static void cleanUp();
    static PerfettoTracingSingleton* mInstance;
    static pthread_rwlock_t mInstanceLock;

private:
    PerfettoTracingSingleton();

public:
    static PerfettoTracingSingleton* instance();
    ~PerfettoTracingSingleton();

    bool initialise(unsigned backends);

    bool isTracing() const;

    bool startInProcessTracing(int fd, const std::string &categoryFilter,
                               size_t maxSize = SIZE_MAX);
    bool startTracing(int fd, const std::string &categoryFilter,
                      size_t maxSize = SIZE_MAX);
 
    void stopInProcessTracing();

    bool flushInProcessTrace(const std::chrono::milliseconds &timeout);

private:
    mutable std::mutex mLock;
    bool mInitialised;
    unsigned mBackends;

    int mTraceFileFd;
    std::unique_ptr<perfetto::TracingSession> mInProcessSession;

};


#endif // PERFETTOTRACINGSINGLETON_H
