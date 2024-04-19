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

#include "ClientStub.h"
#include "Constants.h"
#include "controlmodule.pb.h"
#include "mediakeysmodule.pb.h"
#include "mediapipelinemodule.pb.h"
#include "webaudioplayermodule.pb.h"

namespace firebolt::rialto::server::ct
{
ClientStub::~ClientStub()
{
    disconnect();
}

std::shared_ptr<::firebolt::rialto::ipc::IChannel> ClientStub::getChannel()
{
    return m_ipcChannel;
}

bool ClientStub::connect()
{
    m_ipcChannel = ipc::IChannelFactory::createFactory()->createChannel(kSocketName);
    if (!m_ipcChannel)
    {
        return false;
    }
    setupSubscriptions<firebolt::rialto::PlaybackStateChangeEvent, firebolt::rialto::NetworkStateChangeEvent,
                       firebolt::rialto::PositionChangeEvent, firebolt::rialto::NeedMediaDataEvent, firebolt::rialto::QosEvent,
                       firebolt::rialto::BufferUnderflowEvent, firebolt::rialto::PlaybackErrorEvent,
                       firebolt::rialto::SetLogLevelsEvent, firebolt::rialto::SourceFlushedEvent,
                       firebolt::rialto::WebAudioPlayerStateEvent, firebolt::rialto::ApplicationStateChangeEvent,
                       firebolt::rialto::PingEvent, firebolt::rialto::LicenseRequestEvent,
                       firebolt::rialto::LicenseRenewalEvent, firebolt::rialto::KeyStatusesChangedEvent>(m_ipcChannel);
    m_ipcThread = std::thread(&ClientStub::ipcThread, this);
    return true;
}

void ClientStub::disconnect()
{
    if (m_ipcChannel)
    {
        m_ipcChannel->disconnect();
    }
    if (m_ipcThread.joinable())
    {
        m_ipcThread.join();
    }
    if (m_ipcChannel)
    {
        teardownSubscriptions(m_ipcChannel);
        m_ipcChannel.reset();
    }
}

void ClientStub::ipcThread()
{
    while (m_ipcChannel->process())
    {
        m_ipcChannel->wait(-1);
    }
}
} // namespace firebolt::rialto::server::ct
