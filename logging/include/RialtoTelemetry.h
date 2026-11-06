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

#include <cstdint>
#include <type_traits>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef TELEMETRY_ENABLED
#include <telemetry_busmessage_sender.h>
#endif

#ifdef __cplusplus
}
#endif

void TELEMETRY_INIT(const char* component);
void TELEMETRY_UNINIT();
void TELEMETRY_EVENT_STRING(const char* marker, const char* value);
void TELEMETRY_EVENT_FLOAT(const char* marker, float value);
void TELEMETRY_EVENT_INT_IMPL(const char *marker, int value);
template <typename T>
void TELEMETRY_EVENT_INT(const char* marker, T value)
{
    static_assert(std::is_enum<T>::value, "TELEMETRY_EVENT_INT expects an enum");
    TELEMETRY_EVENT_INT_IMPL(marker, static_cast<int>(value));
}
void TELEMETRY_EVENT_COUNT(const char* marker);

namespace T2
{
    enum SERVERMGR : uint32_t
    {
        SERVER_DEADLOCK = 1,
        SERVER_CRASH,
        STATE_CHANGE_FAIL
    }; /* SERVERMGR */

    namespace CLIENT
    {
        enum SHM : uint32_t
        {
            INIT_FAIL = 1,
            GET_FAIL
        };

        enum IPC : uint32_t
        {
            SERVER_CRASH = 1,          
            MEDIA_PLAYER_CREATE_FAIL,
            MEDIA_KEY_CREATE_FAIL,
            WEB_AUDIO_PLAYER_CREATE_FAIL
        };
    } /*CLIENT*/

    namespace SERVER
    {
        enum INIT : uint32_t
        {
            AMS_SERVER_NULL = 1,
            AMS_CLIENT_NULL,
            SMS_SERVER_NULL,
            SMS_ADD_SOCKET_FAIL,
        };

        enum OCDM : uint32_t
        {
            MEDIA_SESH_FAIL = 1,
            GEN_REQ_FAIL,
            LOAD_SESH_FAIL,
            DRM_HDR_SET_FAIL,
            KEY_SELECT_FAIL,
            ERR_CALLBACK,
        };

        enum SHM : uint32_t
        {
            INIT_FAIL = 1,
            MAP_FAIL
        };

        enum PIPELINE : uint32_t
        {
            LOAD_GST_PLAYER_FAIL = 1,
            CAST_SRC_FAIL,
            PIPELINE_NULL,
            GST_ERR
        };

        enum TEXTTRACK : uint32_t 
        {
            CREATE_INTERFACE_FAIL = 1,
            DISPLAY_SESSION_FAIL,
            PAUSE_FAIL,
            RESUME_FAIL,
            MUTE_FAIL,
            UNMUTE_FAIL,
            SET_POSITION_FAIL,
            SEND_DATA_FAIL,
            SET_WEBVTT_FAIL,
            SET_TTML_FAIL,
            SET_CC_FAIL
        };

        enum PLAYBACK : uint32_t
        {
            GST_OBJ_ERR = 1,
            SET_SRC_FAIL,
            GET_VIDEO_SEG_FAIL,
            GET_AUDIO_SEG_FAIL,
            SEEK_FAIL,
            SWITCH_AUDIO_FAIL,
            PLAYER_SERVICE_INACTIVE,
            CDM_SERVICE_INACTIVE,
            WEB_AUDIO_PLAYER_CREATE_FAIL,
            WEB_AUDIO_PIPELINE_CREATE_FAIL,
            APPSRC_CREATE_FAIL,
            BUFFER_FAIL,
            LARGE_QUEUED_FRAMES,
            WEBAUDIO_SHM_INFO_NULL
        };

        enum SESSION : uint32_t
        {
            NOT_EXIST = 1,
            CREATE_FAIL,
            CREATE_WEBAUDIO_FAIL,
            MEDIA_KEY_HANDLE_FAIL,
            MEDIA_KEY_HANDLE_MKSID_FAIL
        };
    } /* SERVER */

} /* T2 */

#endif /* RIALTO_TELEMETRY_H_ */
