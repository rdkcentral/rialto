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

#ifdef RIALTO_TELEMETRY_SUPPORT
#include <telemetry_busmessage_sender.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef RIALTO_TELEMETRY_SUPPORT

#define TELEMETRY_INIT(component)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        t2_init(reinterpret_cast<char *>(component));                                                                  \
    } while (0)

#define TELEMETRY_UNINIT()                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        t2_uninit();                                                                                                   \
    } while (0)

#define TELEMETRY_EVENT_STRING(marker, value)                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        t2_event_s(reinterpret_cast<char *>(marker), reinterpret_cast<char *>(value));                                 \
    } while (0)

#define TELEMETRY_EVENT_FLOAT(marker, value)                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        t2_event_f(reinterpret_cast<char *>(marker), static_cast<double>(value));                                      \
    } while (0)

#define TELEMETRY_EVENT_INT(marker, value)                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        t2_event_d(reinterpret_cast<char *>(marker), static_cast<int>(value));                                         \
    } while (0)

#else /* stub implementation if telemetry not enabled */

#define TELEMETRY_INIT(component)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#define TELEMETRY_UNINIT()                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#define TELEMETRY_EVENT_STRING(m, v)                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#define TELEMETRY_EVENT_FLOAT(m, v)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#define TELEMETRY_EVENT_INT(m, v)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)

#endif /* RIALTO_TELEMETRY_SUPPORT */

#ifdef __cplusplus
}
#endif

#endif /* RIALTO_TELEMETRY_H_ */
