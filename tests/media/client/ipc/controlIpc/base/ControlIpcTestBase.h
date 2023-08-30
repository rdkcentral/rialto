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

#ifndef CONTROL_IPC_TEST_BASE_H_
#define CONTROL_IPC_TEST_BASE_H_

#include "ControlClientMock.h"
#include "ControlIpc.h"
#include "EventThreadFactoryMock.h"
#include "EventThreadMock.h"
#include "IpcClientMock.h"
#include "IpcModuleBase.h"
#include "SchemaVersion.h"
#include <gtest/gtest.h>
#include <memory>
#include <optional>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::common;
using namespace firebolt::rialto::ipc;

using ::testing::StrictMock;

class ControlIpcTestBase : public IpcModuleBase, public ::testing::Test
{
protected:
    StrictMock<ControlClientMock> m_controlClientMock;
    std::shared_ptr<StrictMock<EventThreadFactoryMock>> m_eventThreadFactoryMock;
    std::unique_ptr<StrictMock<EventThreadMock>> m_eventThread;
    StrictMock<EventThreadMock> *m_eventThreadMock;

    // ControlIpc object
    std::shared_ptr<IControlIpc> m_controlIpc;

    // Callbacks
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_notifyApplicationStateCb;
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_pingCb;

    // Variables
    const int m_kHandleId{0};

    ControlIpcTestBase();
    ~ControlIpcTestBase();

    void createControlIpc();
    void destroyControlIpc();
    bool registerClient(const std::optional<firebolt::rialto::common::SchemaVersion> &schemaVersion = std::nullopt);
    void expectSubscribeEvents();
    void expectUnsubscribeEvents();
};

#endif // CONTROL_IPC_TEST_BASE_H_
