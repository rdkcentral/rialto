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
#include <cstring>
#include <cstdio>

#ifdef __cplusplus
extern "C"
{
#endif

#define RIALTO_TELEMETRY_LOG_MIL(fmt, args...) RIALTO_LOG_MIL(RIALTO_COMPONENT_SERVER, fmt, ##args)

#ifdef __cplusplus
}
#endif

#ifdef RIALTO_TELEMETRY_SUPPORT

void TELEMETRY_INIT(const char* component)
{
    RIALTO_TELEMETRY_LOG_MIL("Telemetry initialized for: %s", component);
    printf("Telemetry initialized for: %s", component);
    // Copy into a mutable buffer: t2_init takes char* and may write to it,
    // so passing a const/string-literal directly is undefined behavior.
    char buf[64];
    strncpy(buf, component, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    t2_init(buf);
}

void TELEMETRY_UNINIT()
{
    t2_uninit();
}

void TELEMETRY_EVENT_STRING(const char* marker, const char* value)
{
    RIALTO_LOG_MIL(RIALTO_COMPONENT_SERVER, "Telemetry marker: %s, value: %s", marker, value);
    printf("Telemetry marker: %s, value: %s", marker, value);
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

#endif /* RIALTO_TELEMETRY_SUPPORT */
