#include "EventBus.h"

namespace rialto_refplayer
{

void EventBus::setClient(std::weak_ptr<IRefPlayerClient> client)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_client = std::move(client);
}

void EventBus::publish(const Event &event)
{
    std::weak_ptr<IRefPlayerClient> weak;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        weak = m_client;
    }
    if (const auto client = weak.lock())
    {
        client->onEvent(event);
    }
}

} // namespace rialto_refplayer
