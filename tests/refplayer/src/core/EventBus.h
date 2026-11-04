#pragma once

#include "rialto_refplayer/RefPlayerEvent.h"

#include <memory>
#include <mutex>

namespace rialto_refplayer
{

// Typed publish to the single registered client. The client is copied under the
// lock and called after it is released, so callers may publish from any thread, but
// they must not hold a library lock of their own across publish().
class EventBus
{
public:
    void setClient(std::weak_ptr<IRefPlayerClient> client);
    void publish(const Event &event);

private:
    std::mutex m_mutex;
    std::weak_ptr<IRefPlayerClient> m_client;
};

} // namespace rialto_refplayer
