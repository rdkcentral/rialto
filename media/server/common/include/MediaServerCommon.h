/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef FIREBOLT_RIALTO_MEDIA_SERVER_COMMON_H_
#define FIREBOLT_RIALTO_MEDIA_SERVER_COMMON_H_

/**
 * @file MediaCommon.h
 *
 * The definition of the Rialto Media Server Common types
 *
 */

namespace firebolt::rialto::server
{
/**
 * @brief The State of the Session Server.
 */
enum class SessionServerState
{
    UNINITIALIZED,
    INACTIVE,
    ACTIVE,
    NOT_RUNNING,
    ERROR
};

/**
 * @brief The type of media playback.
 */
enum class MediaPlaybackType
{
    GENERIC,
    WEB_AUDIO
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_MEDIA_SERVER_COMMON_H_
