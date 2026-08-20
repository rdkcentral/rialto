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

#ifndef RIALTO_TELEMETRY_H_
#define RIALTO_TELEMETRY_H_

#include "RialtoLogging.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RIALTO_TELEMETRY_LOG_MIL(fmt, args...) RIALTO_LOG_MIL(RIALTO_COMPONENT_TELEMETRY, fmt, ##args)

#ifdef TELEMETRY_ENABLED

#include <telemetry_busmessage_sender.h>

void TELEMETRY_INIT(const char* component);
void TELEMETRY_UNINIT();
void TELEMETRY_EVENT_STRING(const char* marker, const char* value);
void TELEMETRY_EVENT_FLOAT(const char* marker, float value);
void TELEMETRY_EVENT_INT(const char* marker, int value);

#else

inline void TELEMETRY_INIT(const char* component) { }
inline void TELEMETRY_UNINIT() {}
inline void TELEMETRY_EVENT_STRING(const char* marker, const char* value) { }
inline void TELEMETRY_EVENT_FLOAT(const char* marker, float value) {}
inline void TELEMETRY_EVENT_INT(const char* marker, int value) {}

#endif /* TELEMETRY_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* RIALTO_TELEMETRY_H_ */