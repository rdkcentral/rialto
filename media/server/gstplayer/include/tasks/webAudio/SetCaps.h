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

#ifndef FIREBOLT_RIALTO_SERVER_WEBAUDIO_SET_CAPS_H_
#define FIREBOLT_RIALTO_SERVER_WEBAUDIO_SET_CAPS_H_

#include "IGlibWrapper.h"
#include "IGstWrapper.h"
#include "IPlayerTask.h"
#include "WebAudioPlayerContext.h"
#include <memory>
#include <string>

namespace firebolt::rialto::server::webaudio
{
class SetCaps : public IPlayerTask
{
public:
    SetCaps(WebAudioPlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper,
            std::shared_ptr<IGlibWrapper> glibWrapper, const std::string &audioMimeType, const WebAudioConfig *config);
    ~SetCaps() override;
    void execute() const override;

private:
    GstCaps *createCapsFromMimeType() const;

    WebAudioPlayerContext &m_context;
    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::shared_ptr<IGlibWrapper> m_glibWrapper;
    const std::string m_audioMimeType;
    WebAudioConfig m_config;
};
} // namespace firebolt::rialto::server::webaudio

#endif // FIREBOLT_RIALTO_SERVER_WEBAUDIO_SET_CAPS_H_
