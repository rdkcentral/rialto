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

#ifndef FIREBOLT_RIALTO_SERVER_CT_CONFIGURE_ACTION_H_
#define FIREBOLT_RIALTO_SERVER_CT_CONFIGURE_ACTION_H_

#include "IBlockingClosure.h"
#include "IStub.h"
#include <IIpcControllerFactory.h>
#include <functional>
#include <gtest/gtest.h>

namespace firebolt::rialto::server::ct
{
template <typename Action> class ConfigureAction
{
public:
    ConfigureAction(IStub &stub)
    {
        m_serviceStub = std::make_unique<typename Action::Stub>(stub.getChannel().get());
        EXPECT_TRUE(m_serviceStub);
    }

    ConfigureAction &send(const typename Action::RequestType &request)
    {
        auto ipcController = createRpcController();
        auto blockingClosure = createBlockingClosure();
        (m_serviceStub.get()->*Action::m_kFunction)(ipcController.get(), &request, &m_response, blockingClosure.get());
        blockingClosure->wait();
        m_actionFailed = ipcController->Failed();
        return *this;
    }

    ConfigureAction &expectFailure()
    {
        EXPECT_TRUE(m_actionFailed);
        return *this;
    }

    ConfigureAction &expectSuccess()
    {
        EXPECT_FALSE(m_actionFailed);
        return *this;
    }

    ConfigureAction &matchResponse(std::function<void(const typename Action::ResponseType &)> matcher)
    {
        matcher(m_response);
        return *this;
    }

private:
    std::shared_ptr<firebolt::rialto::ipc::IBlockingClosure> createBlockingClosure()
    {
        auto factory = firebolt::rialto::ipc::IBlockingClosureFactory::createFactory();
        return factory->createBlockingClosureSemaphore();
    }

    std::shared_ptr<google::protobuf::RpcController> createRpcController()
    {
        return firebolt::rialto::ipc::IControllerFactory::createFactory()->create();
    }

private:
    std::unique_ptr<typename Action::Stub> m_serviceStub;
    typename Action::ResponseType m_response;
    bool m_actionFailed{false};
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_CONFIGURE_ACTION_H_
