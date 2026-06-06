#include "Module.h"
#include "HdmiConnectionMonitor.h"
#include "RialtoServerLogging.h"

#include <WPEFramework/core/core.h>

#include <iostream>

using namespace WPEFramework;

namespace firebolt::rialto::server
{

namespace
{
    constexpr const char *THUNDER_ACCESS = "127.0.0.1:9998";
    constexpr const char *HDCP_CALLSIGN = "org.rdk.HdcpProfile";
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
        RIALTO_SERVER_LOG_ERROR("[Rialto] Unsubscribing HdcpProfile event");

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
    RIALTO_SERVER_LOG_ERROR("[Rialto] Connecting to Thunder");

    /* ===== Set Thunder access point ===== */
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), _T(THUNDER_ACCESS));

    /* ===== JSONRPC LinkType (Thunder R4) ===== */
    m_hdcpConnection = std::make_unique<
        JSONRPC::LinkType<Core::JSON::IElement>>(
        HDCP_CALLSIGN,
        false);

    if (!m_hdcpConnection)
    {
        RIALTO_SERVER_LOG_ERROR("[Rialto] ERROR: Failed to create HDCP connection");
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

    if (status == Core::ERROR_NONE && response.HasLabel("isConnected"))
    {
        bool connected = response["isConnected"].Boolean();
        m_hdmiConnected.store(connected);

        RIALTO_SERVER_LOG_ERROR("[Rialto] Initial HDMI state = %s",(connected ? "CONNECTED" : "DISCONNECTED"));
    }
}

/* ================= Subscribe ================= */
void HdmiConnectionMonitor::subscribe()
{
    if (!m_hdcpConnection)
        return;

    RIALTO_SERVER_LOG_ERROR("[Rialto] Subscribing to event");

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
	JsonObject resultContext = params["HDCPStatus"].Object();
	std::string output;
	params.ToString(output);

	RIALTO_SERVER_LOG_ERROR("[Rialto] Event received with data :%s",output.c_str());
    bool connected = false;
    bool hdcpStatus = false;

    if (!(resultContext.HasLabel("isConnected") || resultContext.HasLabel("hdcpReason")))
    {
        RIALTO_SERVER_LOG_ERROR("[Rialto] ERROR: Missing connected");
        return;
    }

    bool isConnected = resultContext["isConnected"].Boolean();
    if (strcmp(resultContext["hdcpReason"].String().c_str(),"2")){
        RIALTO_SERVER_LOG_ERROR("[Rialto] HDCP reason is 2");
 	   hdcpStatus = true;
    }
    if (isConnected && hdcpStatus) {
        RIALTO_SERVER_LOG_ERROR("[Rialto] HDMI connected");
	    connected = true;
    }
    bool prev = m_hdmiConnected.exchange(connected);

    if (prev == connected) {
        RIALTO_SERVER_LOG_ERROR("[Rialto] Previous was same as connected");
        //return;
    }

    RIALTO_SERVER_LOG_ERROR("[Rialto] HDMI state changed → %s",(connected ? "CONNECTED" : "DISCONNECTED") );

    if (m_callback)
    {
        RIALTO_SERVER_LOG_ERROR("[Rialto] Callback sent");
        m_callback(connected);
    }
}

} // namespace firebolt::rialto::server
