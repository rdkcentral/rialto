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

namespace firebolt::rialto::wrappers
{
std::shared_ptr<ITextTrackPluginWrapper> TextTrackPluginWrapperFactory::getTextTrackPluginWrapper()
{
    return std::make_shared<TextTrackPluginWrapper>();
}

TextTrackPluginWrapper::~TextTrackPluginWrapper()
{
#ifdef RIALTO_ENABLE_TEXT_TRACK
    m_textTrackPlugin.Close(WPEFramework::RPC::CommunicationTimeOut);
#endif // RIALTO_ENABLE_TEXT_TRACK
}

std::uint32_t TextTrackPluginWrapper::open()
{
#ifdef RIALTO_ENABLE_TEXT_TRACK
    return m_textTrackPlugin.Open(WPEFramework::RPC::CommunicationTimeOut, m_textTrackPlugin.Connector(),
                                  "org.rdk.TextTrack");
#else
    return 1234; // wez to zmien
#endif // RIALTO_ENABLE_TEXT_TRACK
}

bool TextTrackPluginWrapper::isOperational() const
{
#ifdef RIALTO_ENABLE_TEXT_TRACK
    return m_textTrackPlugin.IsOperational();
#else
    return false;
#endif // RIALTO_ENABLE_TEXT_TRACK
}

std::shared_ptr<ITextTrackWrapper> TextTrackPluginWrapper::interface()
{
#ifdef RIALTO_ENABLE_TEXT_TRACK
    WPEFramework::Exchange::ITextTrack *textTrackControlInterface = m_textTrackPlugin.Interface();
    if (textTrackControlInterface)
    {
        return std::make_shared<TextTrackWrapper>(textTrackControlInterface);
    }
#endif // RIALTO_ENABLE_TEXT_TRACK
    return nullptr;
}
} // namespace firebolt::rialto::wrappers
