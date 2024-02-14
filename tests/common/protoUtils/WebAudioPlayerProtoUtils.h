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

#ifndef WEB_AUDIO_PLAYER_PROTO_UTILS_H_
#define WEB_AUDIO_PLAYER_PROTO_UTILS_H_

#include "MediaCommon.h"
#include "webaudioplayermodule.pb.h"

inline firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState
convertWebAudioPlayerState(const firebolt::rialto::WebAudioPlayerState &state)
{
    switch (state)
    {
    case firebolt::rialto::WebAudioPlayerState::UNKNOWN:
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_UNKNOWN;
    case firebolt::rialto::WebAudioPlayerState::IDLE:
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_IDLE;
    case firebolt::rialto::WebAudioPlayerState::PLAYING:
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_PLAYING;
    case firebolt::rialto::WebAudioPlayerState::PAUSED:
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_PAUSED;
    case firebolt::rialto::WebAudioPlayerState::END_OF_STREAM:
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_END_OF_STREAM;
    case firebolt::rialto::WebAudioPlayerState::FAILURE:
        return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_FAILURE;
    }
    return firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_UNKNOWN;
}

#endif // WEB_AUDIO_PLAYER_PROTO_UTILS_H_
