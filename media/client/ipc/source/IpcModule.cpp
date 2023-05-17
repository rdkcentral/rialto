/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "IpcModule.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto::client
{
IpcModule::IpcModule(IIpcClient &ipcClient) : m_ipc{ipcClient}
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
}

IpcModule::~IpcModule()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
}

bool IpcModule::unsubscribeFromAllEvents()
{
    if (!m_ipcChannel)
    {
        return false;
    }

    bool result = true;
    for (auto it = m_eventTags.begin(); it != m_eventTags.end(); it++)
    {
        if (!m_ipcChannel->unsubscribe(*it))
        {
            result = false;
        }
    }
    m_eventTags.clear();

    return result;
}

bool IpcModule::attachChannel()
{
    // get the channel
    m_ipcChannel = m_ipc.getChannel();
    if (!m_ipcChannel)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to get the ipc channel");
        return false;
    }

    // check channel connected
    if (!m_ipcChannel->isConnected())
    {
        RIALTO_CLIENT_LOG_ERROR("Ipc channel not connected");
        m_ipcChannel.reset();
        return false;
    }

    // create the RPC stubs
    if (!createRpcStubs())
    {
        RIALTO_CLIENT_LOG_ERROR("Could not create the ipc module stubs");
        m_ipcChannel.reset();
        return false;
    }

    // install listeners
    if (!subscribeToEvents())
    {
        RIALTO_CLIENT_LOG_ERROR("Could not subscribe to ipc module events");
        unsubscribeFromAllEvents();
        m_ipcChannel.reset();
        return false;
    }

    return true;
}

void IpcModule::detachChannel()
{
    // uninstalls listeners
    if (!unsubscribeFromAllEvents())
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to unsubscribe to some ipc module events, this can lead to core dumps in IPC");
    }

    // remove IPC
    m_ipcChannel.reset();
}

bool IpcModule::reattachChannelIfRequired()
{
    if ((nullptr == m_ipcChannel) || (!m_ipcChannel->isConnected()))
    {
        RIALTO_CLIENT_LOG_INFO("Ipc channel no longer connected, attach new channel");
        detachChannel();
        if (!attachChannel())
        {
            RIALTO_CLIENT_LOG_ERROR("Failed attach to the ipc channel");
            return false;
        }
    }

    return true;
}

}; // namespace firebolt::rialto::client
