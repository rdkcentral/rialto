/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#ifndef PRIVATE_METRICS_MODULE_MOCK_H_
#define PRIVATE_METRICS_MODULE_MOCK_H_

#include "privatemetricsmodule.pb.h"
#include <gmock/gmock.h>

class PrivateMetricsModuleMock : public ::firebolt::rialto::PrivateMetricsModule
{
public:
    MOCK_METHOD(void, notifyClientReady,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::NotifyClientReadyRequest *request,
                 ::firebolt::rialto::NotifyClientReadyResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, reportClientMetrics,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::ReportClientMetricsRequest *request,
                 ::firebolt::rialto::ReportClientMetricsResponse *response, ::google::protobuf::Closure *done));

    void defaultReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        done->Run();
    }
};

#endif // PRIVATE_METRICS_MODULE_MOCK_H_
