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

#ifndef FIREBOLT_RIALTO_CLIENT_CONTROL_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_CONTROL_IPC_H_

#include "IControlClient.h"
#include "IControlIpc.h"
#include "IEventThread.h"
#include "IIpcClient.h"
#include "IpcModule.h"
#include <memory>

#include "controlmodule.pb.h"

namespace firebolt::rialto::client
{
class ControlIpc;

/**
 * @brief The definition of the ControlIpcAccessor.
 */
class ControlIpcAccessor : public IControlIpcAccessor, public IIpcClientAccessor
{
public:
    ~ControlIpcAccessor() override = default;
    IControlIpc &getControlIpc() const override;
    IIpcClient &getIpcClient() const override;
};

/**
 * @brief The definition of the ControlIpc.
 */
class ControlIpc : public IControlIpc, public IIpcClient, public IpcModule
{
public:
    /**
     * @brief The constructor.
     */
    ControlIpc(const std::shared_ptr<ipc::IChannelFactory> &ipcChannelFactory,
               const std::shared_ptr<ipc::IControllerFactory> &ipcControllerFactory,
               const std::shared_ptr<ipc::IBlockingClosureFactory> &blockingClosureFactory,
               const std::shared_ptr<common::IEventThreadFactory> &eventThreadFactory);

    /**
     * @brief Virtual destructor.
     */
    virtual ~ControlIpc();

    bool getSharedMemory(int32_t &fd, uint32_t &size) override;
    bool registerClient(IControlClient *client) override;
    bool unregisterClient(IControlClient *client) override;

    std::shared_ptr<ipc::IChannel> getChannel() const override;
    std::shared_ptr<ipc::IBlockingClosure> createBlockingClosure() override;
    std::shared_ptr<google::protobuf::RpcController> createRpcController() override;

private:
    bool createRpcStubs() override;
    bool subscribeToEvents() override;
    bool connect();
    bool disconnect();

    /**
     * @brief The processing loop for the ipc thread.
     */
    void ipcThread();

    /**
     * @brief Handler for a application state update from the server.
     *
     * @param[in] event : The app state changed event structure.
     */
    void onApplicationStateUpdated(const std::shared_ptr<firebolt::rialto::ApplicationStateChangeEvent> &event);

    /**
     * @brief Handler for a ping from the server.
     *
     * @param[in] event : The ping event structure.
     */
    void onPing(const std::shared_ptr<firebolt::rialto::PingEvent> &event);

private:
    /**
     * @brief The ipc thread.
     */
    std::thread m_ipcThread;

    /**
     * @brief The connected ipc communication channel.
     */
    std::shared_ptr<ipc::IChannel> m_ipcChannel;

    /**
     * @brief Factory for creating the ipc controllers.
     */
    std::shared_ptr<ipc::IControllerFactory> m_ipcControllerFactory;

    /**
     * @brief Factory for creating a connected ipc channel.
     */
    std::shared_ptr<ipc::IChannelFactory> m_ipcChannelFactory;

    /**
     * @brief Factory for creating a blocking closure.
     */
    std::shared_ptr<ipc::IBlockingClosureFactory> m_blockingClosureFactory;

    /**
     * @brief Control client for handling messages from server
     */
    IControlClient *m_controlClient;

    /**
     * @brief Thread for handling media player events from the server.
     */
    std::unique_ptr<common::IEventThread> m_eventThread;

    /**
     * @brief RPC stubs for the Control APIs.
     */
    std::shared_ptr<::firebolt::rialto::ControlModule_Stub> m_controlStub;
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_CONTROL_IPC_H_
