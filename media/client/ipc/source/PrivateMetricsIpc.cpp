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

#include "PrivateMetricsIpc.h"
#include "IpcClient.h"
#include "RialtoClientLogging.h"
#include <cinttypes>
#include <unistd.h>

namespace
{
const char *sampleReasonToString(::firebolt::rialto::MetricsSampleReason reason)
{
    switch (reason)
    {
    case firebolt::rialto::METRICS_SAMPLE_REASON_CONNECTED:
        return "CONNECTED";
    case firebolt::rialto::METRICS_SAMPLE_REASON_PERIODIC:
        return "PERIODIC";
    case firebolt::rialto::METRICS_SAMPLE_REASON_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}
} // namespace

namespace firebolt::rialto::client
{
std::shared_ptr<IPrivateMetricsIpcFactory> IPrivateMetricsIpcFactory::createFactory()
{
    return PrivateMetricsIpcFactory::createFactory();
}

std::shared_ptr<PrivateMetricsIpcFactory> PrivateMetricsIpcFactory::createFactory()
{
    std::shared_ptr<PrivateMetricsIpcFactory> factory;

    try
    {
        factory = std::make_shared<PrivateMetricsIpcFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the rialto private metrics ipc factory, reason: %s", e.what());
    }

    return factory;
}

std::shared_ptr<IPrivateMetricsIpc> PrivateMetricsIpcFactory::createPrivateMetricsIpc(IPrivateMetricsIpcClient *client)
{
    auto &ipcClient{IIpcClientAccessor::instance().getIpcClient()};
    return std::make_shared<PrivateMetricsIpc>(client, ipcClient,
                                               firebolt::rialto::common::IEventThreadFactory::createFactory());
}

PrivateMetricsIpc::PrivateMetricsIpc(IPrivateMetricsIpcClient *client, IIpcClient &ipcClient,
                                     const std::shared_ptr<common::IEventThreadFactory> &eventThreadFactory)
    : IpcModule(ipcClient), m_privateMetricsIpcClient{client},
      m_eventThread(eventThreadFactory->createEventThread("rialto-metrics-events"))
{
    RIALTO_CLIENT_LOG_MIL("Initialising private metrics IPC, pid=%d", getpid());
    if (!attachChannel())
    {
        throw std::runtime_error("Failed attach to the ipc channel");
    }
    if (!notifyClientReady())
    {
        throw std::runtime_error("Failed to notify private metrics readiness");
    }
}

PrivateMetricsIpc::~PrivateMetricsIpc()
{
    RIALTO_CLIENT_LOG_MIL("Terminating private metrics IPC, pid=%d", getpid());
    detachChannel();
    m_eventThread.reset();
}

bool PrivateMetricsIpc::reportClientMetrics(std::uint64_t sampleId, std::uint32_t reason, const std::string &appName,
                                            std::uint32_t processId, std::uint64_t monotonicTimeMs,
                                            std::uint64_t epochTimeMs, std::uint64_t processCpuTimeMs)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::ReportClientMetricsRequest request;
    auto metrics{request.mutable_metrics()};
    metrics->set_sample_id(sampleId);
    metrics->set_reason(static_cast<firebolt::rialto::MetricsSampleReason>(reason));
    metrics->set_app_name(appName);
    metrics->set_process_id(processId);
    metrics->set_monotonic_time_ms(monotonicTimeMs);
    metrics->set_epoch_time_ms(epochTimeMs);
    metrics->set_process_cpu_time_ms(processCpuTimeMs);

    RIALTO_CLIENT_LOG_MIL("Reporting metrics sample=%" PRIu64 ", reason=%s, app='%s', pid=%u, cpu_ms=%" PRIu64,
                          sampleId,
                          sampleReasonToString(static_cast<firebolt::rialto::MetricsSampleReason>(reason)),
                          appName.c_str(), processId, processCpuTimeMs);

    firebolt::rialto::ReportClientMetricsResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_privateMetricsStub->reportClientMetrics(ipcController.get(), &request, &response, blockingClosure.get());

    blockingClosure->wait();

    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to report client metrics due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    RIALTO_CLIENT_LOG_INFO("Reported metrics sample=%" PRIu64 ", reason=%s", sampleId,
                           sampleReasonToString(static_cast<firebolt::rialto::MetricsSampleReason>(reason)));

    return true;
}

bool PrivateMetricsIpc::notifyClientReady()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    RIALTO_CLIENT_LOG_MIL("Notifying server that private metrics IPC is ready, pid=%d", getpid());

    firebolt::rialto::NotifyClientReadyRequest request;
    firebolt::rialto::NotifyClientReadyResponse response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_privateMetricsStub->notifyClientReady(ipcController.get(), &request, &response, blockingClosure.get());

    blockingClosure->wait();

    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to notify private metrics readiness due to '%s'",
                                ipcController->ErrorText().c_str());
        return false;
    }

    RIALTO_CLIENT_LOG_MIL("Server acknowledged private metrics IPC readiness, pid=%d", getpid());

    return true;
}

bool PrivateMetricsIpc::createRpcStubs(const std::shared_ptr<ipc::IChannel> &ipcChannel)
{
    m_privateMetricsStub = std::make_shared<::firebolt::rialto::PrivateMetricsModule_Stub>(ipcChannel.get());
    return static_cast<bool>(m_privateMetricsStub);
}

bool PrivateMetricsIpc::subscribeToEvents(const std::shared_ptr<ipc::IChannel> &ipcChannel)
{
    if (!ipcChannel)
    {
        return false;
    }

    int eventTag = ipcChannel->subscribe<firebolt::rialto::MetricsSampleRequestEvent>(
        [this](const std::shared_ptr<firebolt::rialto::MetricsSampleRequestEvent> &event)
        { m_eventThread->add(&PrivateMetricsIpc::onMetricsSampleRequested, this, event); });
    if (eventTag < 0)
    {
        return false;
    }
    m_eventTags.push_back(eventTag);

    RIALTO_CLIENT_LOG_MIL("Subscribed to private metrics sample requests, pid=%d, event_tag=%d", getpid(), eventTag);

    return true;
}

void PrivateMetricsIpc::onMetricsSampleRequested(
    const std::shared_ptr<firebolt::rialto::MetricsSampleRequestEvent> &event)
{
    if (!m_privateMetricsIpcClient)
    {
        RIALTO_CLIENT_LOG_WARN("No private metrics client registered");
        return;
    }
    RIALTO_CLIENT_LOG_MIL("Received metrics sample request sample=%" PRIu64 ", reason=%s, pid=%d", event->sample_id(),
                          sampleReasonToString(event->reason()), getpid());
    m_privateMetricsIpcClient->reportClientMetrics(event->sample_id(), event->reason());
}
} // namespace firebolt::rialto::client
