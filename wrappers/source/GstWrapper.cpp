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

#include "GstWrapper.h"
#include <atomic>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <string>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <thread>
#include<chrono>

void debugLog(const std::string& message)
{
    static std::ofstream logFile("/opt/logs/rialto_debug.log", std::ios::app);

    // Get timestamp with microsecond precision
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm tm_info;
   // localtime_r(&tv.tv_sec, &tm_info);  Thread-safe version
    gmtime_r(&tv.tv_sec, &tm_info);  // UTC instead of local time
    char timeBuffer[64];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &tm_info);

    logFile << "[" << timeBuffer << "." << std::setfill('0') << std::setw(6) << tv.tv_usec
            << " PID:" << getpid() << " TID:" << syscall(SYS_gettid) << "] "
            << message << std::endl;

    logFile.flush();
}
namespace firebolt::rialto::wrappers
{
std::shared_ptr<IGstWrapper> GstWrapperFactory::getGstWrapper()
{  
    debugLog("inside GstWrapperFactory::getGstWrapper");
    static std::shared_ptr<IGstWrapper> gstWrapper{};
    if (!gstWrapper)
    {   
        debugLog("creating GstWrapper instance");

        // --- repro-only barrier: hold whichever thread arrives first until a second thread
        // also arrives (or 50ms elapses), so both threads call make_shared back-to-back ---
        static std::atomic<int> raceArrivals{0};
        int myArrivalNumber = ++raceArrivals;
        if (myArrivalNumber == 1)
        {
            auto waitStart = std::chrono::steady_clock::now();
            while (raceArrivals.load() < 2 &&
                   std::chrono::steady_clock::now() - waitStart < std::chrono::milliseconds(50))
            {
                std::this_thread::yield();
            }
        }
        // --- end repro-only barrier ---

        try
        {
            debugLog("inside try block of GstWrapperFactory::getGstWrapper");
            gstWrapper = std::make_shared<GstWrapper>();
        }
        catch (const std::exception &e)
        {
            debugLog("exception caught in GstWrapperFactory::getGstWrapper");
        }
    }
    debugLog("returning GstWrapper instance");
    return gstWrapper;
}

void GstWrapper::gstCapsSetSimple(GstCaps *caps, const gchar *field, ...) const
{
    va_list vl;
    va_start(vl, field);
    gst_caps_set_simple_valist(caps, field, vl);
    va_end(vl);
}

GstStructure *GstWrapper::gstStructureNew(const gchar *name, const gchar *firstfield, ...) const
{
    GstStructure *structure{nullptr};
    va_list vl;
    va_start(vl, firstfield);
    structure = gst_structure_new_valist(name, firstfield, vl);
    va_end(vl);
    return structure;
}

void GstWrapper::gstStructureSet(GstStructure *structure, const gchar *firstname, ...) const
{
    va_list vl;
    va_start(vl, firstname);
    gst_structure_set_valist(structure, firstname, vl);
    va_end(vl);
}
}; // namespace firebolt::rialto::wrappers
