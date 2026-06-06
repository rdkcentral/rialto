#include "Module.h"
#include "HdmiConnectionMonitor.h"

#include <WPEFramework/core/core.h>

#include <iostream>

using namespace WPEFramework;

namespace firebolt::rialto::server
{

namespace
{
    constexpr const char *THUNDER_ACCESS = "127.0.0.1:9998";
    constexpr const char *HDCP_CALLSIGN = "HdcpProfile";
    constexpr const char *EVENT_NAME = "onDisplayConnectionChanged";
    constexpr uint32_t kDefaultWaitTimeMs = 10000;
}

/* ================= Constructor ================= */
HdmiConnectionMonitor::HdmiConnectionMonitor(Callback cb)
    : m_callback(std::move(cb))
{
    connect();
}

/* ================= Destructor ================= */
HdmiConnectionMonitor::~HdmiConnectionMonitor()
{
    if (m_hdcpConnection)
    {
        std::cout << "[Rialto] Unsubscribing HdcpProfile event\n";

        m_hdcpConnection->Unsubscribe(kDefaultWaitTimeMs, EVENT_NAME);
        m_hdcpConnection.reset();
    }
}

/* ================= Public ================= */
bool HdmiConnectionMonitor::isConnected() const
{
    return m_hdmiConnected.load();
}

/* ================= Connect (AAMP-style) ================= */
void HdmiConnectionMonitor::connect()
{
    std::cout << "[Rialto] Connecting to Thunder\n";

    /* ===== Set Thunder access point ===== */
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), _T(THUNDER_ACCESS));

    /* ===== JSONRPC LinkType (Thunder R4) ===== */
    m_hdcpConnection = std::make_unique<
        JSONRPC::LinkType<Core::JSON::IElement>>(
        HDCP_CALLSIGN,
        false);

    if (!m_hdcpConnection)
    {
        std::cerr << "[Rialto] ERROR: Failed to create HDCP connection\n";
        return;
    }

    subscribe();

    /* ===== Get initial HDMI state (AAMP style) ===== */
    Core::JSON::VariantContainer parameters;
    Core::JSON::VariantContainer response;

    uint32_t status = m_hdcpConnection->Invoke(
        _T("getHDCPStatus"),
        parameters,
        response);

    if (status == Core::ERROR_NONE && response.HasLabel("connected"))
    {
        bool connected = response["connected"].Boolean();
        m_hdmiConnected.store(connected);

        std::cout << "[Rialto] Initial HDMI state = "
                  << (connected ? "CONNECTED" : "DISCONNECTED") << "\n";
    }
}

/* ================= Subscribe ================= */
void HdmiConnectionMonitor::subscribe()
{
    if (!m_hdcpConnection)
        return;

    std::cout << "[Rialto] Subscribing to event\n";

    m_hdcpConnection->Subscribe<Core::JSON::VariantContainer>(
        kDefaultWaitTimeMs,
        EVENT_NAME,
        [this](const Core::JSON::VariantContainer &params)
        {
            onDisplayConnectionChanged(params);
        });
}

/* ================= Event Handler ================= */
void HdmiConnectionMonitor::onDisplayConnectionChanged(
    const Core::JSON::VariantContainer &params)
{
    if (!params.HasLabel("connected"))
    {
        std::cerr << "[Rialto] ERROR: Missing 'connected'\n";
        return;
    }

    bool connected = params["connected"].Boolean();

    bool prev = m_hdmiConnected.exchange(connected);

    if (prev == connected)
        return;

    std::cout << "[Rialto] HDMI state changed → "
              << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;

    if (m_callback)
    {
        m_callback(connected);
    }
}

} // namespace firebolt::rialto::server
