/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef WEB_AUDIO_PLAYER_PROTO_REQUEST_MATCHERS_H_
#define WEB_AUDIO_PLAYER_PROTO_REQUEST_MATCHERS_H_

// #include "MediaCommon.h"
#include "webaudioplayermodule.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

MATCHER_P3(createWebAudioPlayerRequestMatcher, audioMimeType, priority, config, "")
{
    const ::firebolt::rialto::CreateWebAudioPlayerRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::CreateWebAudioPlayerRequest *>(arg);
    return (kRequest->audio_mime_type() == audioMimeType && kRequest->priority() == priority);
}

MATCHER_P(destroyWebAudioPlayerRequestMatcher, webAudioPlayerHandle, "")
{
    const ::firebolt::rialto::DestroyWebAudioPlayerRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::DestroyWebAudioPlayerRequest *>(arg);
    return (kRequest->web_audio_player_handle());
}

#endif // WEB_AUDIO_PLAYER_PROTO_REQUEST_MATCHERS_H_
