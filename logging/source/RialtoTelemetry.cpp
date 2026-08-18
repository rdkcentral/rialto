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

#include "RialtoLogging.h"
#include "stdio.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RIALTO_TELEMETRY_LOG_MIL(fmt, args...) RIALTO_LOG_MIL(RIALTO_COMPONENT_TELEMETRY, fmt, ##args)

#ifdef __cplusplus
}
#endif


#ifdef TELEMETRY_ENABLED

void TELEMETRY_INIT(const char* component)
{
    /*debug*/fprintf(stderr, "bvanav-dbg: Telemetry initialized for %s \n", component);
    RIALTO_TELEMETRY_LOG_MIL("Telemetry initialized for %s", component);
    t2_init(const_cast<char*>(component));
}

void TELEMETRY_UNINIT()
{
    RIALTO_TELEMETRY_LOG_MIL("Telemetry uninitialized");
    t2_uninit();
}

void TELEMETRY_EVENT_STRING(const char* marker, const char* value)
{
    /*debug*/fprintf(stderr, "bvanav-dbg: Telemetry String - Marker: %s, Value: %s \n", marker, value);
    t2_event_s(const_cast<char*>(marker), const_cast<char*>(value));
}

void TELEMETRY_EVENT_FLOAT(const char* marker, float value)
{
    t2_event_f(const_cast<char*>(marker), static_cast<double>(value));
}

void TELEMETRY_EVENT_INT(const char* marker, int value)
{
    t2_event_d(const_cast<char*>(marker), static_cast<int>(value));
}

#endif /* TELEMETRY_ENABLED */