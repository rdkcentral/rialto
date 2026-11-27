#include "RialtoPlayerJSBinding.h"

#include "JSEventListener.h"
#include "JSUtils.h"

#include <cmath>
#include <optional>
#include <string>
#include <vector>

namespace rialto_refplayer
{
namespace jsbinding
{

namespace
{

const char *const kClassName = "RialtoMediaPlayer";
const char *const kAampName = "AAMPMediaPlayer";
const JSPropertyAttributes kMethodAttributes = kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly;

// Per-context state, from LoadJS to UnloadJS.
struct ContextState
{
    JSGlobalContextRef context{nullptr};
    JSClassRef classRef{nullptr};
    JSObjectRef constructor{nullptr};
    // Players still alive. Each entry is removed by finalize or by UnloadJS.
    std::vector<std::shared_ptr<PlayerCore>> instances;
    // Callbacks that finalize detached before UnloadJS ran. UnloadJS unprotects them.
    std::vector<JSObjectRef> orphanedCallbacks;
};

// Guards g_contexts. Finalize takes it on a GC thread, so no JavaScriptCore call is made while it is held.
std::mutex g_registryMutex;
std::vector<ContextState> g_contexts;

ContextState *findContextLocked(JSGlobalContextRef context)
{
    for (ContextState &state : g_contexts)
    {
        if (state.context == context)
        {
            return &state;
        }
    }
    return nullptr;
}

void eraseInstanceLocked(ContextState &state, const std::shared_ptr<PlayerCore> &core)
{
    for (auto it = state.instances.begin(); it != state.instances.end(); ++it)
    {
        if (*it == core)
        {
            state.instances.erase(it);
            return;
        }
    }
}

void registerInstance(JSGlobalContextRef context, const std::shared_ptr<PlayerCore> &core)
{
    std::lock_guard<std::mutex> lock(g_registryMutex);
    if (ContextState *state = findContextLocked(context))
    {
        state->instances.push_back(core);
    }
}

JSClassRef classForContext(JSContextRef ctx)
{
    const JSGlobalContextRef context = JSContextGetGlobalContext(ctx);
    std::lock_guard<std::mutex> lock(g_registryMutex);
    const ContextState *state = findContextLocked(context);
    return state ? state->classRef : nullptr;
}

// Idempotent. Stops delivery, removes the pending dispatch source and destroys the library object.
// Returns the callbacks that were registered. The caller unprotects them, now or later.
// No JavaScriptCore call is made here, so finalize may use it.
std::vector<JSObjectRef> shutdownCore(PlayerCore &core)
{
    std::lock_guard<std::mutex> lock(core.shutdownMutex);
    if (core.shutdownDone)
    {
        return std::vector<JSObjectRef>();
    }
    core.shutdownDone = true;
    core.listener->close();
    std::vector<JSObjectRef> callbacks = core.listener->detachAll();
    core.player->destroy();
    return callbacks;
}

// The private data of thisObject. Sets an Error and returns nullptr for any other object.
RialtoPlayerPrivate *selfOf(JSContextRef ctx, JSObjectRef thisObject, const char *method, JSValueRef *exception)
{
    const JSClassRef classRef = classForContext(ctx);
    if (thisObject && classRef && JSValueIsObjectOfClass(ctx, thisObject, classRef))
    {
        auto *self = static_cast<RialtoPlayerPrivate *>(JSObjectGetPrivate(thisObject));
        if (self && self->core)
        {
            return self;
        }
    }
    throwError(ctx, exception, std::string("Can only call ") + method + "() on instances of RialtoMediaPlayer");
    return nullptr;
}

void logIfFailed(const char *method, Result result)
{
    if (result != Result::Ok)
    {
        logWarning(std::string(method) + "() returned " + rialto_refplayer::toString(result));
    }
}

// Bool-returning commands report true only for Result::Ok.
JSValueRef boolResult(JSContextRef ctx, const char *method, Result result)
{
    logIfFailed(method, result);
    return JSValueMakeBoolean(ctx, result == Result::Ok);
}

// A missing or non-numeric argument becomes NaN, which the library's range checks reject.
double numberArgument(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[], size_t index, JSValueRef *exception)
{
    if (index >= argumentCount)
    {
        return std::nan("");
    }
    return JSValueToNumber(ctx, arguments[index], exception);
}

bool booleanArgument(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[], size_t index)
{
    return index < argumentCount && JSValueToBoolean(ctx, arguments[index]);
}

// Copies a string property when the object has one. Undefined, null and non-strings leave the value unchanged.
bool readString(JSContextRef ctx, JSObjectRef object, const char *name, std::string &out, JSValueRef *exception)
{
    const JSValueRef value = getProperty(ctx, object, name, exception);
    if (*exception)
    {
        return false;
    }
    if (JSValueIsString(ctx, value))
    {
        out = toStdString(ctx, value, exception);
    }
    return *exception == nullptr;
}

// Copies the keys present in config into options. Unknown keys are ignored. Tokens are never logged.
bool applyConfig(JSContextRef ctx, JSObjectRef config, Options &options, JSValueRef *exception)
{
    const JSValueRef interval = getProperty(ctx, config, "progressReportingInterval", exception);
    if (*exception)
    {
        return false;
    }
    if (JSValueIsNumber(ctx, interval))
    {
        // AAMP's interval is in seconds. The library's throttle is in milliseconds.
        const double seconds = JSValueToNumber(ctx, interval, exception);
        if (*exception)
        {
            return false;
        }
        int64_t milliseconds = 0;
        if (seconds >= 0.0 && toInt64(std::round(seconds * 1000.0), milliseconds))
        {
            options.progressIntervalMs = milliseconds;
        }
        else
        {
            logWarning("progressReportingInterval ignored: not a finite, non-negative number");
        }
    }

    return readString(ctx, config, "authToken", options.authToken, exception) &&
           readString(ctx, config, "accountId", options.accountId, exception) &&
           readString(ctx, config, "deviceId", options.deviceId, exception);
}

// ---- Method table: every row of the design's command table ----

JSValueRef jsInitConfig(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "initConfig", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    if (argumentCount < 1 || !JSValueIsObject(ctx, arguments[0]))
    {
        throwError(ctx, exception, "initConfig() expects a config object");
        return JSValueMakeUndefined(ctx);
    }

    PlayerCore &core = *self->core;
    const JSObjectRef config = JSValueToObject(ctx, arguments[0], exception);
    if (!applyConfig(ctx, config, core.options, exception))
    {
        return JSValueMakeUndefined(ctx);
    }

    if (!core.initialized)
    {
        // The first call connects. A failure leaves the object uninitialised, so the next call tries again.
        const Result result = core.player->initialize(core.options);
        if (result != Result::Ok)
        {
            throwError(ctx, exception, std::string("initConfig failed: ") + rialto_refplayer::toString(result));
            return JSValueMakeUndefined(ctx);
        }
        core.initialized = true;
        return JSValueMakeUndefined(ctx);
    }

    // Later calls only merge the config. They never reconnect.
    const Result result = core.player->updateConfig(core.options);
    if (result != Result::Ok)
    {
        logWarning(std::string("updateConfig failed: ") + rialto_refplayer::toString(result));
    }
    return JSValueMakeUndefined(ctx);
}

JSValueRef jsLoad(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "load", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    if (argumentCount < 1 || !JSValueIsString(ctx, arguments[0]))
    {
        throwError(ctx, exception, "load() expects a URL string");
        return JSValueMakeUndefined(ctx);
    }

    const std::string url = toStdString(ctx, arguments[0], exception);
    // Autoplay defaults to true, as in AAMP. A boolean second argument overrides it.
    bool autoplay = true;
    if (argumentCount >= 2 && JSValueIsBoolean(ctx, arguments[1]))
    {
        autoplay = JSValueToBoolean(ctx, arguments[1]);
    }
    logIfFailed("load", self->core->player->load(url, autoplay));
    return JSValueMakeUndefined(ctx);
}

JSValueRef jsPlay(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "play", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    logIfFailed("play", self->core->player->play());
    return JSValueMakeUndefined(ctx);
}

JSValueRef jsPause(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "pause", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    logIfFailed("pause", self->core->player->pause());
    return JSValueMakeUndefined(ctx);
}

JSValueRef jsStop(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "stop", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    logIfFailed("stop", self->core->player->stop());
    return JSValueMakeUndefined(ctx);
}

JSValueRef jsSeek(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "seek", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    if (argumentCount < 1)
    {
        throwError(ctx, exception, "seek() expects a position in seconds");
        return JSValueMakeUndefined(ctx);
    }

    const double seconds = JSValueToNumber(ctx, arguments[0], exception);
    if (*exception)
    {
        return JSValueMakeUndefined(ctx);
    }
    // Seconds to milliseconds, truncated toward zero.
    int64_t positionMs = 0;
    if (!toInt64(seconds * 1000.0, positionMs))
    {
        logWarning("seek() ignored: position is not a finite number");
        return JSValueMakeUndefined(ctx);
    }
    logIfFailed("seek", self->core->player->seek(positionMs));
    return JSValueMakeUndefined(ctx);
}

JSValueRef jsGetCurrentPosition(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "getCurrentPosition", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    return JSValueMakeNumber(ctx, static_cast<double>(self->core->player->positionMs()) / 1000.0);
}

JSValueRef jsGetDurationSec(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "getDurationSec", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    return JSValueMakeNumber(ctx, static_cast<double>(self->core->player->durationMs()) / 1000.0);
}

JSValueRef jsGetCurrentState(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "getCurrentState", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    return JSValueMakeNumber(ctx, static_cast<double>(static_cast<int>(self->core->player->state())));
}

JSValueRef jsAddEventListener(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "addEventListener", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    if (argumentCount < 2)
    {
        throwError(ctx, exception, "addEventListener() - 2 arguments required.");
        return JSValueMakeUndefined(ctx);
    }

    const std::string name = toStdString(ctx, arguments[0], exception);
    if (!isCallable(ctx, arguments[1]))
    {
        throwError(ctx, exception, "Failed to execute addEventListener() for event " + name + " - parameter 2 is not a function");
        return JSValueMakeUndefined(ctx);
    }

    EventType type = EventType::StateChanged;
    if (!eventTypeFromName(name, type))
    {
        // Unknown names are accepted and never fire, as in AAMP.
        return JSValueMakeUndefined(ctx);
    }

    const JSObjectRef callback = JSValueToObject(ctx, arguments[1], exception);
    if (self->core->listener->addCallback(type, callback))
    {
        JSValueProtect(ctx, callback);
    }
    return JSValueMakeUndefined(ctx);
}

JSValueRef jsRemoveEventListener(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "removeEventListener", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    if (argumentCount < 2)
    {
        throwError(ctx, exception, "removeEventListener() - 2 arguments required.");
        return JSValueMakeUndefined(ctx);
    }

    const std::string name = toStdString(ctx, arguments[0], exception);
    if (!isCallable(ctx, arguments[1]))
    {
        throwError(ctx, exception, "Failed to execute removeEventListener() for event " + name + " - parameter 2 is not a function");
        return JSValueMakeUndefined(ctx);
    }

    EventType type = EventType::StateChanged;
    if (!eventTypeFromName(name, type))
    {
        return JSValueMakeUndefined(ctx);
    }

    const JSObjectRef callback = JSValueToObject(ctx, arguments[1], exception);
    if (self->core->listener->removeCallback(type, callback))
    {
        JSValueUnprotect(ctx, callback);
    }
    return JSValueMakeUndefined(ctx);
}

JSValueRef jsRelease(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "release", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    PlayerCore &core = *self->core;
    logIfFailed("stop", core.player->stop());
    // The library object is gone after this, so no callback is wanted. Release them all.
    for (JSObjectRef callback : shutdownCore(core))
    {
        JSValueUnprotect(ctx, callback);
    }
    return JSValueMakeUndefined(ctx);
}

JSValueRef jsSetPlaybackRate(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "setPlaybackRate", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    const double rate = numberArgument(ctx, argumentCount, arguments, 0, exception);
    if (*exception)
    {
        return JSValueMakeUndefined(ctx);
    }
    return boolResult(ctx, "setPlaybackRate", self->core->player->setPlaybackRate(rate));
}

JSValueRef jsGetSupportedPlaybackSpeeds(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "getSupportedPlaybackSpeeds", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    return makeNumberArray(ctx, self->core->player->supportedSpeeds());
}

JSValueRef jsSetVolume(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "setVolume", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    const double volume = numberArgument(ctx, argumentCount, arguments, 0, exception);
    if (*exception)
    {
        return JSValueMakeUndefined(ctx);
    }
    return boolResult(ctx, "setVolume", self->core->player->setVolume(volume));
}

JSValueRef jsGetVolume(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "getVolume", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    const std::optional<double> volume = self->core->player->volume();
    if (!volume)
    {
        return JSValueMakeNull(ctx);
    }
    return JSValueMakeNumber(ctx, *volume);
}

JSValueRef jsSetVideoRect(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "setVideoRect", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    uint32_t values[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < 4; ++i)
    {
        const double value = numberArgument(ctx, argumentCount, arguments, i, exception);
        if (*exception)
        {
            return JSValueMakeUndefined(ctx);
        }
        if (!toUint32(value, values[i]))
        {
            logWarning("setVideoRect() rejected: coordinates must be non-negative numbers");
            return JSValueMakeBoolean(ctx, false);
        }
    }
    return boolResult(ctx, "setVideoRect", self->core->player->setVideoWindow(values[0], values[1], values[2], values[3]));
}

JSValueRef jsSetVideoMute(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "setVideoMute", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    return boolResult(ctx, "setVideoMute", self->core->player->setVideoMute(booleanArgument(ctx, argumentCount, arguments, 0)));
}

JSValueRef jsGetAvailableTextTracks(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "getAvailableTextTracks", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    // undefined, not an empty string: the wrapper branches on it.
    const std::optional<std::string> tracks = self->core->player->textTracksJson();
    if (!tracks)
    {
        return JSValueMakeUndefined(ctx);
    }
    return makeString(ctx, *tracks);
}

JSValueRef jsSetTextTrack(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "setTextTrack", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    const double index = numberArgument(ctx, argumentCount, arguments, 0, exception);
    if (*exception)
    {
        return JSValueMakeUndefined(ctx);
    }
    int track = 0;
    if (!toInt(index, track))
    {
        logWarning("setTextTrack() rejected: index is not an integer");
        return JSValueMakeBoolean(ctx, false);
    }
    return boolResult(ctx, "setTextTrack", self->core->player->setTextTrack(track));
}

JSValueRef jsGetTextTrack(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "getTextTrack", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    const std::optional<int> track = self->core->player->textTrack();
    if (!track)
    {
        return JSValueMakeNull(ctx);
    }
    return JSValueMakeNumber(ctx, static_cast<double>(*track));
}

JSValueRef jsSetClosedCaptionStatus(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception)
{
    RialtoPlayerPrivate *self = selfOf(ctx, thisObject, "setClosedCaptionStatus", exception);
    if (!self)
    {
        return JSValueMakeUndefined(ctx);
    }
    return boolResult(ctx, "setClosedCaptionStatus", self->core->player->setClosedCaptions(booleanArgument(ctx, argumentCount, arguments, 0)));
}

const JSStaticFunction kMethods[] = {
    {"initConfig", jsInitConfig, kMethodAttributes},
    {"load", jsLoad, kMethodAttributes},
    {"play", jsPlay, kMethodAttributes},
    {"pause", jsPause, kMethodAttributes},
    {"stop", jsStop, kMethodAttributes},
    {"seek", jsSeek, kMethodAttributes},
    {"getCurrentPosition", jsGetCurrentPosition, kMethodAttributes},
    {"getDurationSec", jsGetDurationSec, kMethodAttributes},
    {"getCurrentState", jsGetCurrentState, kMethodAttributes},
    {"addEventListener", jsAddEventListener, kMethodAttributes},
    {"removeEventListener", jsRemoveEventListener, kMethodAttributes},
    {"release", jsRelease, kMethodAttributes},
    {"setPlaybackRate", jsSetPlaybackRate, kMethodAttributes},
    {"getSupportedPlaybackSpeeds", jsGetSupportedPlaybackSpeeds, kMethodAttributes},
    {"setVolume", jsSetVolume, kMethodAttributes},
    {"getVolume", jsGetVolume, kMethodAttributes},
    {"setVideoRect", jsSetVideoRect, kMethodAttributes},
    {"setVideoMute", jsSetVideoMute, kMethodAttributes},
    {"getAvailableTextTracks", jsGetAvailableTextTracks, kMethodAttributes},
    {"setTextTrack", jsSetTextTrack, kMethodAttributes},
    {"getTextTrack", jsGetTextTrack, kMethodAttributes},
    {"setClosedCaptionStatus", jsSetClosedCaptionStatus, kMethodAttributes},
    {nullptr, nullptr, 0},
};

// ---- Object lifecycle ----

JSObjectRef jsConstruct(JSContextRef ctx, JSObjectRef /*constructor*/, size_t /*argumentCount*/, const JSValueRef * /*arguments*/, JSValueRef *exception)
{
    const JSGlobalContextRef context = JSContextGetGlobalContext(ctx);
    const JSClassRef classRef = classForContext(ctx);
    if (!classRef)
    {
        throwError(ctx, exception, "RialtoMediaPlayer is not registered in this context");
        return nullptr;
    }

    // Construction does not connect to Rialto. initConfig() does that, so this cannot fail on server state.
    auto core = std::make_shared<PlayerCore>();
    core->listener = std::make_shared<JSEventListener>(context);
    core->player = rialto_refplayer::createRefPlayer();
    if (!core->player)
    {
        throwError(ctx, exception, "RialtoMediaPlayer: the reference player could not be created");
        return nullptr;
    }
    // The library holds only a weak_ptr. PlayerCore keeps the listener alive.
    core->player->setClient(core->listener);

    auto *self = new RialtoPlayerPrivate{context, core};
    JSObjectRef object = JSObjectMake(ctx, classRef, nullptr);
    if (!JSObjectSetPrivate(object, self))
    {
        shutdownCore(*core);
        delete self;
        throwError(ctx, exception, "RialtoMediaPlayer: could not attach private data");
        return nullptr;
    }
    registerInstance(context, core);
    return object;
}

// GC thread. Only JSObjectGetPrivate touches JavaScriptCore, and it only reads the pointer.
void jsFinalize(JSObjectRef object)
{
    auto *self = static_cast<RialtoPlayerPrivate *>(JSObjectGetPrivate(object));
    if (!self)
    {
        return;
    }
    const std::shared_ptr<PlayerCore> core = self->core;
    std::vector<JSObjectRef> callbacks = shutdownCore(*core);
    {
        std::lock_guard<std::mutex> lock(g_registryMutex);
        if (ContextState *state = findContextLocked(self->context))
        {
            eraseInstanceLocked(*state, core);
            // UnloadJS unprotects these. Without the hand-off they would stay protected until the context is released.
            state->orphanedCallbacks.insert(state->orphanedCallbacks.end(), callbacks.begin(), callbacks.end());
        }
    }
    delete self;
}

void loadContext(void *context)
{
    if (!context)
    {
        return;
    }
    const JSGlobalContextRef global = static_cast<JSGlobalContextRef>(context);
    {
        std::lock_guard<std::mutex> lock(g_registryMutex);
        if (findContextLocked(global))
        {
            return;
        }
    }

    JSClassDefinition definition = kJSClassDefinitionEmpty;
    definition.className = kClassName;
    definition.staticFunctions = kMethods;
    definition.finalize = jsFinalize;
    const JSClassRef classRef = JSClassCreate(&definition);
    const JSObjectRef constructor = JSObjectMakeConstructor(global, classRef, jsConstruct);
    JSValueProtect(global, constructor);

    // AAMPMediaPlayer is never written. It may be present in the same process.
    const JSObjectRef globalObject = JSContextGetGlobalObject(global);
    JSValueRef exception = nullptr;
    const JSValueRef aamp = getProperty(global, globalObject, kAampName, &exception);
    if (!JSValueIsUndefined(global, aamp))
    {
        logWarning("AAMPMediaPlayer is already defined; leaving it untouched");
    }
    setProperty(global, globalObject, kClassName, constructor, kJSPropertyAttributeNone);

    ContextState state;
    state.context = global;
    state.classRef = classRef;
    state.constructor = constructor;
    std::lock_guard<std::mutex> lock(g_registryMutex);
    g_contexts.push_back(std::move(state));
}

void unloadContext(void *context)
{
    if (!context)
    {
        return;
    }
    const JSGlobalContextRef global = static_cast<JSGlobalContextRef>(context);

    ContextState state;
    {
        std::lock_guard<std::mutex> lock(g_registryMutex);
        for (auto it = g_contexts.begin(); it != g_contexts.end(); ++it)
        {
            if (it->context == global)
            {
                state = std::move(*it);
                g_contexts.erase(it);
                break;
            }
        }
    }
    if (!state.context)
    {
        return;
    }

    // Destroy every live instance first, so no player outlives its context.
    for (const std::shared_ptr<PlayerCore> &core : state.instances)
    {
        for (JSObjectRef callback : shutdownCore(*core))
        {
            JSValueUnprotect(global, callback);
        }
    }
    state.instances.clear();
    for (JSObjectRef callback : state.orphanedCallbacks)
    {
        JSValueUnprotect(global, callback);
    }

    JSValueUnprotect(global, state.constructor);
    setProperty(global, JSContextGetGlobalObject(global), kClassName, JSValueMakeUndefined(global), kJSPropertyAttributeNone);
    JSClassRelease(state.classRef);
}

} // namespace

} // namespace jsbinding
} // namespace rialto_refplayer

// The exported entry points. Each one only forwards to the implementation above.
void RialtoPlayer_LoadJS(void *context)
{
    rialto_refplayer::jsbinding::loadContext(context);
}

void RialtoPlayer_UnloadJS(void *context)
{
    rialto_refplayer::jsbinding::unloadContext(context);
}
