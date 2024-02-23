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

#include "webaudioplayermodule.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

MATCHER_P3(createWebAudioPlayerRequestMatcher, audioMimeType, priority, config, "")
{
    const ::firebolt::rialto::CreateWebAudioPlayerRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::CreateWebAudioPlayerRequest *>(arg);
    return ((kRequest->audio_mime_type() == audioMimeType) && (kRequest->priority() == priority) &&
            (kRequest->config().pcm().rate() == config->pcm.rate &&
             kRequest->config().pcm().channels() == config->pcm.channels &&
             kRequest->config().pcm().sample_size() == config->pcm.sampleSize &&
             kRequest->config().pcm().is_big_endian() == config->pcm.isBigEndian &&
             kRequest->config().pcm().is_signed() == config->pcm.isSigned &&
             kRequest->config().pcm().is_float() == config->pcm.isFloat));
}

MATCHER_P(destroyWebAudioPlayerRequestMatcher, webAudioPlayerHandle, "")
{
    const ::firebolt::rialto::DestroyWebAudioPlayerRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::DestroyWebAudioPlayerRequest *>(arg);
    return (kRequest->web_audio_player_handle() == webAudioPlayerHandle);
}

MATCHER_P(webAudioGetDeviceInfoRequestMatcher, webAudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioGetDeviceInfoRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioGetDeviceInfoRequest *>(arg);
    return (kRequest->web_audio_player_handle() == webAudioPlayerHandle);
}

MATCHER_P(webAudioPlayRequestMatcher, webAudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioPlayRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioPlayRequest *>(arg);
    return (kRequest->web_audio_player_handle() == webAudioPlayerHandle);
}

MATCHER_P(webAudioPauseRequestMatcher, webAudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioPauseRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioPauseRequest *>(arg);
    return (kRequest->web_audio_player_handle() == webAudioPlayerHandle);
}

MATCHER_P(webAudioSetEosRequestMatcher, webAudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioSetEosRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioSetEosRequest *>(arg);
    return (kRequest->web_audio_player_handle() == webAudioPlayerHandle);
}

MATCHER_P(webAudioGetBufferAvailableRequestMatcher, webAudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioGetBufferAvailableRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioGetBufferAvailableRequest *>(arg);
    return (kRequest->web_audio_player_handle() == webAudioPlayerHandle);
}

MATCHER_P2(webAudioWriteBufferRequestMatcher, webAudioPlayerHandle, numberOfFrames, "")
{
    const ::firebolt::rialto::WebAudioWriteBufferRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioWriteBufferRequest *>(arg);
    return (kRequest->web_audio_player_handle() == webAudioPlayerHandle && kRequest->number_of_frames() == numberOfFrames);
}

MATCHER_P(webAudioGetBufferDelayRequestMatcher, webAudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioGetBufferDelayRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioGetBufferDelayRequest *>(arg);
    return (kRequest->web_audio_player_handle() == webAudioPlayerHandle);
}

MATCHER_P2(webAudioSetVolumeRequestMatcher, webAudioPlayerHandle, volume, "")
{
    const ::firebolt::rialto::WebAudioSetVolumeRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioSetVolumeRequest *>(arg);
    return ((kRequest->web_audio_player_handle() == webAudioPlayerHandle) && (kRequest->volume() == volume));
}

MATCHER_P(webAudioGetVolumeRequestMatcher, webAudioPlayerHandle, "")
{
    const ::firebolt::rialto::WebAudioGetVolumeRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::WebAudioGetVolumeRequest *>(arg);
    return (kRequest->web_audio_player_handle() == webAudioPlayerHandle);
}
#endif // WEB_AUDIO_PLAYER_PROTO_REQUEST_MATCHERS_H_
