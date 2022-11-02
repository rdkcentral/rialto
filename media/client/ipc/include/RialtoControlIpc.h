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

#ifndef FIREBOLT_RIALTO_CLIENT_RIALTO_CONTROL_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_RIALTO_CONTROL_IPC_H_

#include "IIpcClient.h"
#include "IRialtoControlIpc.h"
#include <memory>

#include "rialtocontrolmodule.pb.h"

namespace firebolt::rialto::client
{
class RialtoControlIpc;

/**
 * @brief IRialtoControlIpc factory class definition.
 */
class RialtoControlIpcFactory : public IRialtoControlIpcFactory, public IIpcClientFactory
{
public:
    RialtoControlIpcFactory() = default;
    ~RialtoControlIpcFactory() override = default;

    std::shared_ptr<IRialtoControlIpc> getRialtoControlIpc() override;

    std::shared_ptr<IIpcClient> getIpcClient() override;

    /**
     * @brief Create the generic rialto control factory object.
     *
     * @retval the generic rialto control factory instance or null on error.
     */
    static std::shared_ptr<RialtoControlIpcFactory> createFactory();

protected:
    /**
     * @brief Weak pointer to the singleton rialto control object.
     */
    static std::weak_ptr<RialtoControlIpc> m_rialtoControlIpc;

    /**
     * @brief Create generic object.
     *
     * @retval the generic rialto control ipc instance or null on error.
     */
    std::shared_ptr<RialtoControlIpc> getGeneric();
};

/**
 * @brief The definition of the RialtoControlIpc.
 */
class RialtoControlIpc : public IRialtoControlIpc, public IIpcClient
{
public:
    /**
     * @brief The constructor.
     */
    RialtoControlIpc(const std::shared_ptr<ipc::IChannelFactory> &ipcChannelFactory,
                     const std::shared_ptr<ipc::IControllerFactory> &ipcControllerFactory,
                     const std::shared_ptr<ipc::IBlockingClosureFactory> &blockingClosureFactory);

    /**
     * @brief Virtual destructor.
     */
    virtual ~RialtoControlIpc();

    bool connect() override;

    bool disconnect() override;

    bool getSharedMemory(int32_t &fd, uint32_t &size) override;

    std::shared_ptr<ipc::IChannel> getChannel() const override;

    std::shared_ptr<ipc::IBlockingClosure> createBlockingClosure() override;

    std::shared_ptr<google::protobuf::RpcController> createRpcController() override;

protected:
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
     * @brief RPC stubs for the RialtoControl APIs.
     */
    std::shared_ptr<::firebolt::rialto::RialtoControlModule_Stub> m_rialtoControlStub;

    /**
     * @brief Factory for creating a connected ipc channel.
     */
    std::shared_ptr<ipc::IChannelFactory> m_ipcChannelFactory;

    /**
     * @brief Factory for creating a blocking closure.
     */
    std::shared_ptr<ipc::IBlockingClosureFactory> m_blockingClosureFactory;

    /**
     * @brief The processing loop for the ipc thread.
     */
    void ipcThread();
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_RIALTO_CONTROL_IPC_H_
