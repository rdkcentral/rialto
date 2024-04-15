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

#ifndef FIREBOLT_RIALTO_WEB_AUDIO_PLAYER_CLIENT_MOCK_H_
#define FIREBOLT_RIALTO_WEB_AUDIO_PLAYER_CLIENT_MOCK_H_

#include <gmock/gmock.h>
#include <memory>

#include "IWebAudioPlayerClient.h"
#include "MediaCommon.h"

namespace firebolt::rialto
{
class WebAudioPlayerClientMock : public IWebAudioPlayerClient
{
public:
    WebAudioPlayerClientMock() = default;
    virtual ~WebAudioPlayerClientMock() = default;
    MOCK_METHOD(void, notifyState, (WebAudioPlayerState state), (override));
};
} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_WEB_AUDIO_PLAYER_CLIENT_MOCK_H_
