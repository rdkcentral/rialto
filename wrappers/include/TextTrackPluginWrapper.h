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
#ifndef FIREBOLT_RIALTO_WRAPPERS_TEXT_TRACK_PLUGIN_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_TEXT_TRACK_PLUGIN_WRAPPER_H_

#ifndef MODULE_NAME
#define MODULE_NAME TextTrackClosedCaptionsStyleClient
#endif

#include "ITextTrackPluginWrapper.h"
#include <com/com.h>
#include <core/core.h>
#include <interfaces/ITextTrack.h>
#include <memory>
#include <plugins/Types.h>

namespace firebolt::rialto::wrappers
{

class TextTrackPluginWrapper : public ITextTrackPluginWrapper
{
public:
    TextTrackPluginWrapper() = default;
    ~TextTrackPluginWrapper() override;

    std::uint32_t open() override;
    bool isOperational() const override;
    std::shared_ptr<ITextTrackWrapper> interface() override;

private:
    WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::ITextTrack> m_textTrackPlugin;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_TEXT_TRACK_PLUGIN_WRAPPER_H_
