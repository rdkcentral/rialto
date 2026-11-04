#pragma once

#include "JSEventListener.h"

#include "rialto_refplayer/IRefPlayer.h"
#include "rialto_refplayer/RefPlayerTypes.h"

#include <JavaScriptCore/JavaScript.h>

#include <memory>
#include <mutex>

// The only two symbols this library exports. RialtoPlayerJSBindings.map enforces that.
#define RIALTO_JSBINDING_EXPORT __attribute__((visibility("default")))

namespace rialto_refplayer
{
namespace jsbinding
{

// Everything one player owns. Shared between the JS object and the instance registry, so
// that UnloadJS can shut a player down without touching the JS object's private data,
// which finalize frees on a GC thread.
struct PlayerCore
{
    std::shared_ptr<IRefPlayer> player;
    std::shared_ptr<JSEventListener> listener;
    Options options;          // JS thread only
    bool initialized{false};  // JS thread only
    std::mutex shutdownMutex;
    bool shutdownDone{false};
};

// Private data of a RialtoMediaPlayer object. Created by the constructor and freed by
// finalize. It is never touched by UnloadJS.
struct RialtoPlayerPrivate
{
    JSGlobalContextRef context; // identity only; not retained
    std::shared_ptr<PlayerCore> core;
};

} // namespace jsbinding
} // namespace rialto_refplayer

extern "C" {
// JSGlobalContextRef passed as void*. Called from the WebProcess injected-bundle hook.
RIALTO_JSBINDING_EXPORT void RialtoPlayer_LoadJS(void *context);
RIALTO_JSBINDING_EXPORT void RialtoPlayer_UnloadJS(void *context);
}
