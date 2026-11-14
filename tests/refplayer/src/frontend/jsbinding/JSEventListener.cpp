#include "JSEventListener.h"

#include "JSUtils.h"

namespace rialto_refplayer
{
namespace jsbinding
{

namespace
{

// Owns one reference to the listener for as long as its idle source is pending.
struct DispatchHolder
{
    std::shared_ptr<JSEventListener> listener;
};

void destroyDispatchHolder(gpointer data)
{
    delete static_cast<DispatchHolder *>(data);
}

} // namespace

bool eventTypeFromName(const std::string &name, EventType &type)
{
    if (name == "playbackStateChanged")
    {
        type = EventType::StateChanged;
    }
    else if (name == "playbackProgressUpdate")
    {
        type = EventType::ProgressUpdate;
    }
    else if (name == "playbackFailed")
    {
        type = EventType::Failed;
    }
    else if (name == "bufferingChanged")
    {
        type = EventType::BufferingChanged;
    }
    else if (name == "speedsChanged")
    {
        type = EventType::SpeedsChanged;
    }
    else if (name == "httpResponseHeader")
    {
        type = EventType::HttpResponseHeader;
    }
    else
    {
        return false;
    }
    return true;
}

JSEventListener::JSEventListener(JSGlobalContextRef context)
    : m_context(context)
{
}

JSEventListener::~JSEventListener()
{
    // Every owner calls detachAll() before the last reference goes away, so no protected
    // callback is left behind here.
}

void JSEventListener::onEvent(const Event &event)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_closed)
    {
        return;
    }

    enqueueLocked(event);
    if (m_sourceId == 0)
    {
        auto *holder = new DispatchHolder{shared_from_this()};
        m_sourceId = g_idle_add_full(G_PRIORITY_DEFAULT, &JSEventListener::dispatchIdle, holder, destroyDispatchHolder);
    }
}

void JSEventListener::enqueueLocked(const Event &event)
{
    // Progress is a snapshot. A newer one replaces the pending one rather than queuing behind it.
    if (event.type == EventType::ProgressUpdate)
    {
        for (Event &queued : m_queue)
        {
            if (queued.type == EventType::ProgressUpdate)
            {
                queued = event;
                return;
            }
        }
    }
    m_queue.push_back(event);
}

bool JSEventListener::addCallback(EventType type, JSObjectRef callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_closed)
    {
        return false;
    }
    for (const Registration &registration : m_registrations)
    {
        // A repeated registration of the same callback is ignored, as AAMP does.
        if (registration.type == type && registration.callback == callback)
        {
            return false;
        }
    }
    m_registrations.push_back(Registration{type, callback});
    return true;
}

bool JSEventListener::removeCallback(EventType type, JSObjectRef callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_registrations.begin(); it != m_registrations.end(); ++it)
    {
        if (it->type == type && it->callback == callback)
        {
            m_registrations.erase(it);
            return true;
        }
    }
    return false;
}

void JSEventListener::close()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_closed = true;
    m_queue.clear();
    if (m_sourceId != 0)
    {
        // Removed under the lock, so drain() cannot clear the ID between the check and the removal.
        g_source_remove(m_sourceId);
        m_sourceId = 0;
    }
}

std::vector<JSObjectRef> JSEventListener::detachAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<JSObjectRef> callbacks;
    callbacks.reserve(m_registrations.size());
    for (const Registration &registration : m_registrations)
    {
        callbacks.push_back(registration.callback);
    }
    m_registrations.clear();
    return callbacks;
}

gboolean JSEventListener::dispatchIdle(gpointer data)
{
    // Hold our own reference for the duration of the call.
    std::shared_ptr<JSEventListener> self = static_cast<DispatchHolder *>(data)->listener;
    self->drain();
    return G_SOURCE_REMOVE;
}

void JSEventListener::drain()
{
    std::deque<Event> batch;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // This source is finishing. An onEvent() during delivery schedules a new one, after this batch.
        m_sourceId = 0;
        batch.swap(m_queue);
    }

    for (const Event &event : batch)
    {
        if (m_closed)
        {
            return;
        }
        deliver(event);
    }
}

void JSEventListener::deliver(const Event &event)
{
    const std::vector<JSObjectRef> callbacks = callbacksFor(event.type);
    if (callbacks.empty())
    {
        return;
    }

    JSObjectRef eventObject = buildEventObject(event);
    // The callbacks can allocate and trigger a collection while the object is still in use.
    JSValueProtect(m_context, eventObject);
    for (JSObjectRef callback : callbacks)
    {
        // A callback may have removed this one, or released the player, during this dispatch.
        if (!isRegistered(event.type, callback))
        {
            continue;
        }

        JSValueRef arguments[1] = {eventObject};
        JSValueRef exception = nullptr;
        JSObjectCallAsFunction(m_context, callback, nullptr, 1, arguments, &exception);
        if (exception)
        {
            logWarning("an event listener threw an exception");
        }
    }
    JSValueUnprotect(m_context, eventObject);
}

std::vector<JSObjectRef> JSEventListener::callbacksFor(EventType type)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<JSObjectRef> callbacks;
    for (const Registration &registration : m_registrations)
    {
        if (registration.type == type)
        {
            callbacks.push_back(registration.callback);
        }
    }
    return callbacks;
}

bool JSEventListener::isRegistered(EventType type, JSObjectRef callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const Registration &registration : m_registrations)
    {
        if (registration.type == type && registration.callback == callback)
        {
            return true;
        }
    }
    return false;
}

JSObjectRef JSEventListener::buildEventObject(const Event &event)
{
    // Field names are AAMP's. The misspelling in positionMiliseconds is load-bearing for the tests.
    JSObjectRef object = JSObjectMake(m_context, nullptr, nullptr);
    switch (event.type)
    {
    case EventType::StateChanged:
        setProperty(m_context, object, "state", JSValueMakeNumber(m_context, static_cast<double>(static_cast<int>(event.state))), kJSPropertyAttributeReadOnly);
        break;
    case EventType::ProgressUpdate:
        setProperty(m_context, object, "positionMiliseconds", JSValueMakeNumber(m_context, static_cast<double>(event.positionMs)), kJSPropertyAttributeReadOnly);
        break;
    case EventType::Failed:
        setProperty(m_context, object, "description", makeString(m_context, event.description), kJSPropertyAttributeReadOnly);
        break;
    case EventType::BufferingChanged:
        setProperty(m_context, object, "buffering", JSValueMakeBoolean(m_context, event.buffering), kJSPropertyAttributeReadOnly);
        break;
    case EventType::SpeedsChanged:
        // AAMP's jsbindings.cpp uses this name. The test app reads event.playbackSpeeds.
        setProperty(m_context, object, "playbackSpeeds", makeNumberArray(m_context, event.speeds), kJSPropertyAttributeReadOnly);
        break;
    case EventType::HttpResponseHeader:
        // AAMP's payload is {header, response}. The library event carries one string.
        setProperty(m_context, object, "header", makeString(m_context, event.headerPayload), kJSPropertyAttributeReadOnly);
        break;
    case EventType::MetricReceived:
        break;
    }
    return object;
}

} // namespace jsbinding
} // namespace rialto_refplayer
