#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "Module.h"
#include <WPEFramework/core/JSON.h>
#include <WPEFramework/websocket/JSONRPCLink.h>

namespace firebolt::rialto::server
{

class HdmiConnectionMonitor
{
public:
    using Callback = std::function<void(bool)>;

    explicit HdmiConnectionMonitor(Callback cb);
    ~HdmiConnectionMonitor();

    bool isConnected() const;

private:
    // Connection + subscription
    void connect();
    void subscribe();

    // Event handler
    void onDisplayConnectionChanged(
        const WPEFramework::Core::JSON::VariantContainer &params);

private:
    // HDMI state
    std::atomic<bool> m_hdmiConnected{false};

    // Callback to pipeline/service
    Callback m_callback;

    // Thunder JSONRPC connection (R4 API)
    std::unique_ptr<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>> m_hdcpConnection;
};

} // namespace firebolt::rialto::server
