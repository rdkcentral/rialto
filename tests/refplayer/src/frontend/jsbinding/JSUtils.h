#pragma once

#include <JavaScriptCore/JavaScript.h>

#include <cstdint>
#include <string>
#include <vector>

namespace rialto_refplayer
{
namespace jsbinding
{

// JSString and JSValue helpers. All of these run on the JS thread.

std::string toStdString(JSContextRef ctx, JSValueRef value, JSValueRef *exception);
JSValueRef makeString(JSContextRef ctx, const std::string &value);
JSObjectRef makeNumberArray(JSContextRef ctx, const std::vector<double> &values);
JSValueRef getProperty(JSContextRef ctx, JSObjectRef object, const char *name, JSValueRef *exception);
void setProperty(JSContextRef ctx, JSObjectRef object, const char *name, JSValueRef value, JSPropertyAttributes attributes);
bool isCallable(JSContextRef ctx, JSValueRef value);
void throwError(JSContextRef ctx, JSValueRef *exception, const std::string &message);

// Number conversions. Each returns false for a value that cannot be represented, so the
// caller never performs an out-of-range float-to-integer conversion.
bool toInt64(double value, int64_t &out);   // truncates toward zero
bool toUint32(double value, uint32_t &out);
bool toInt(double value, int &out);

// Diagnostics on stdout. Never pass token values here.
void logWarning(const std::string &message);

} // namespace jsbinding
} // namespace rialto_refplayer
