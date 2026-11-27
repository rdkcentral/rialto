#pragma once

#include "rialto_refplayer/RefPlayerEvent.h"
#include "rialto_refplayer/RefPlayerTypes.h"

#include <JavaScriptCore/JavaScript.h>
#include <glib.h>

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace rialto_refplayer
{
namespace jsbinding
{

// Maps an AAMP event name to an event type. Returns false for names this binding does not
// deliver. addEventListener accepts those names and they never fire, as in AAMP.
bool eventTypeFromName(const std::string &name, EventType &type);

// Receives library events on any thread and delivers them to JS callbacks on the JS thread.
//
// onEvent() does no JavaScriptCore work. It copies the event into a queue and schedules one
// GLib idle source on the JS thread. The source drains the queue, builds each JS event object
// and calls the callbacks. The JS-thread callers of addCallback/removeCallback own the
// JSValueProtect/JSValueUnprotect calls. The listener never unprotects anything itself.
class JSEventListener : public IRefPlayerClient, public std::enable_shared_from_this<JSEventListener>
{
public:
    explicit JSEventListener(JSGlobalContextRef context);
    ~JSEventListener() override;

    JSEventListener(const JSEventListener &) = delete;
    JSEventListener &operator=(const JSEventListener &) = delete;

    // IRefPlayerClient. Any thread.
    void onEvent(const Event &event) override;

    // JS thread only. Returns true if the callback was added; the caller must then protect it.
    bool addCallback(EventType type, JSObjectRef callback);

    // JS thread only. Returns true if the callback was removed; the caller must then unprotect it.
    bool removeCallback(EventType type, JSObjectRef callback);

    // Any thread. Stops delivery and removes the pending idle source. No JavaScriptCore calls.
    void close();

    // Any thread. Removes every callback and returns them, one entry per registration. The
    // caller must unprotect each entry, now or at context teardown. No JavaScriptCore calls.
    std::vector<JSObjectRef> detachAll();

private:
    struct Registration
    {
        EventType type;
        JSObjectRef callback;
    };

    static gboolean dispatchIdle(gpointer data);
    void enqueueLocked(const Event &event);
    void drain();
    void deliver(const Event &event);
    std::vector<JSObjectRef> callbacksFor(EventType type);
    bool isRegistered(EventType type, JSObjectRef callback);
    JSObjectRef buildEventObject(const Event &event);

    JSGlobalContextRef m_context;
    std::mutex m_mutex;
    std::atomic<bool> m_closed{false};
    std::deque<Event> m_queue;                 // pending events; at most one pending progress
    std::vector<Registration> m_registrations;
    guint m_sourceId{0};                       // pending idle source, 0 if none
};

} // namespace jsbinding
} // namespace rialto_refplayer
