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

#pragma once

//todo: wywalic to do Module.h
#ifndef MODULE_NAME
#define MODULE_NAME TextTrackClosedCaptionsStyleClient
#endif


#include <core/core.h>
#include <com/com.h>
#include <plugins/Types.h>
#include <set>
#include <interfaces/ITextTrack.h>
#include "RialtoServerLogging.h"
#include <mutex>

// class TextTrackSession
// {

// };
//todo: klops, mopzna zrobic, ze TextTrackAccessor bedzie signletonem i sesja z Texttrackiem bedzie utrzymywana nonstop
// i bedzie faktorka do tworzenia sesji, ktora bedzie ustawiala typ sesji WebVTT, TTML, itp i bedzie robila tez reset itd.


class TextTrackAccessor
{
    public: //todo-klops: powinien byc singleton?
    TextTrackAccessor(const std::string &displayName = std::string());
    ~TextTrackAccessor();
    bool pause();
    bool play();
    bool mute(bool mute);
    bool setPosition(uint64_t mediaTimestampMs);
    bool sendData(const std::string &data, int32_t displayOffsetMs = 0);
    bool setSessionWebVTTSelection();
    bool setSessionTTMLSelection();

public:
    static TextTrackAccessor &instance();
    // void subscribe(WPEFramework::Exchange::ITextTrackClosedCaptionsStyle::INotification *notification);
    // void unsubscribe(WPEFramework::Exchange::ITextTrackClosedCaptionsStyle::INotification *notification);

private:
    //void reconnect();
    //template <typename Plugin, typename Interface>
    // bool createControlInterface(Plugin &plugin, Interface &interface, const std::string &controlName)
    // {
    //     static const std::string kTextTrackCallSign = "org.rdk.TextTrack";
    //     bool result = false;

    //     const uint32_t openResult =
    //         plugin.Open(WPEFramework::RPC::CommunicationTimeOut, plugin.Connector(), kTextTrackCallSign);
    //     if (openResult == WPEFramework::Core::ERROR_NONE)
    //     {
    //         if (plugin.IsOperational())
    //         {
    //             interface = plugin.Interface();
    //             if (interface)
    //             {
    //                 RIALTO_SERVER_LOG_INFO("Created %s interface", controlName.c_str());
    //                 result = true;
    //             }
    //             else
    //             {
    //                 RIALTO_SERVER_LOG_ERROR("Failed to create %s interface", controlName.c_str());
    //             }
    //         }
    //         else
    //         {
    //             RIALTO_SERVER_LOG_ERROR("%s plugin is NOT operational", controlName.c_str());
    //         }
    //     }
    //     else
    //     {
    //         RIALTO_SERVER_LOG_ERROR("Failed to open %s plugin; error '%s'", controlName.c_str(),
    //                                 WPEFramework::Core::ErrorToString(openResult));
    //     }

    //     return result;
    // }

    //bool createStyleControlInterface();
    bool textTrackControlInterface();


private:
    // WPEFramework::Exchange::ITextTrackClosedCaptionsStyle *m_styleInterface = nullptr;
    // WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::ITextTrackClosedCaptionsStyle> m_stylePlugin;

    WPEFramework::Exchange::ITextTrack *m_textTrackControlInterface = nullptr;
    WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::ITextTrack> m_textTrackPlugin;
    uint32_t m_sessionId;
    std::mutex m_mutex;
};
