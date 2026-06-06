#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include <WPEFramework/core/JSON.h>
#include <WPEFramework/core/JSONRPC.h>

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

    // Event handler (AAMP-style payload)
    void onDisplayConnectionChanged(
        const WPEFramework::Core::JSON::VariantContainer &params);

private:
    // HDMI state
    std::atomic<bool> m_hdmiConnected{false};

    // Callback to pipeline/service
    Callback m_callback;

    // AAMP-style Thunder connection
    WPEFramework::Core::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *m_hdcpConnection;
};

} // namespace firebolt::rialto::server
