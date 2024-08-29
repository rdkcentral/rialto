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

#include "TextTrackPluginWrapper.h"
#include "TextTrackPluginWrapperFactory.h"
#include "TextTrackWrapper.h"

using namespace WPEFramework;

namespace firebolt::rialto::wrappers
{
std::shared_ptr<ITextTrackPluginWrapper> TextTrackPluginWrapperFactory::getTextTrackPluginWrapper()
{
    return std::make_shared<TextTrackPluginWrapper>();
}

TextTrackPluginWrapper::~TextTrackPluginWrapper()
{
    m_textTrackPlugin.Close(RPC::CommunicationTimeOut);
}

std::uint32_t TextTrackPluginWrapper::open()
{
    return m_textTrackPlugin.Open(WPEFramework::RPC::CommunicationTimeOut, m_textTrackPlugin.Connector(),
                                  "org.rdk.TextTrack");
}

bool TextTrackPluginWrapper::isOperational() const
{
    return m_textTrackPlugin.IsOperational();
}

std::shared_ptr<ITextTrackWrapper> TextTrackPluginWrapper::interface()
{
    WPEFramework::Exchange::ITextTrack *textTrackControlInterface = m_textTrackPlugin.Interface();
    if (textTrackControlInterface)
    {
        return std::make_shared<TextTrackWrapper>(textTrackControlInterface);
    }
    return nullptr;
}
} // namespace firebolt::rialto::wrappers
