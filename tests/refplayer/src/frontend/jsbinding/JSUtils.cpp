#include "JSUtils.h"

#include <cmath>
#include <cstdio>

namespace rialto_refplayer
{
namespace jsbinding
{

std::string toStdString(JSContextRef ctx, JSValueRef value, JSValueRef *exception)
{
    JSStringRef string = JSValueToStringCopy(ctx, value, exception);
    if (!string)
    {
        return std::string();
    }

    const size_t size = JSStringGetMaximumUTF8CStringSize(string);
    std::vector<char> buffer(size);
    JSStringGetUTF8CString(string, buffer.data(), size);
    JSStringRelease(string);
    return std::string(buffer.data());
}

JSValueRef makeString(JSContextRef ctx, const std::string &value)
{
    JSStringRef string = JSStringCreateWithUTF8CString(value.c_str());
    JSValueRef result = JSValueMakeString(ctx, string);
    JSStringRelease(string);
    return result;
}

JSObjectRef makeNumberArray(JSContextRef ctx, const std::vector<double> &values)
{
    std::vector<JSValueRef> items;
    items.reserve(values.size());
    for (double value : values)
    {
        items.push_back(JSValueMakeNumber(ctx, value));
    }
    return JSObjectMakeArray(ctx, items.size(), items.empty() ? nullptr : items.data(), nullptr);
}

JSValueRef getProperty(JSContextRef ctx, JSObjectRef object, const char *name, JSValueRef *exception)
{
    JSStringRef key = JSStringCreateWithUTF8CString(name);
    JSValueRef value = JSObjectGetProperty(ctx, object, key, exception);
    JSStringRelease(key);
    return value;
}

void setProperty(JSContextRef ctx, JSObjectRef object, const char *name, JSValueRef value, JSPropertyAttributes attributes)
{
    JSStringRef key = JSStringCreateWithUTF8CString(name);
    JSObjectSetProperty(ctx, object, key, value, attributes, nullptr);
    JSStringRelease(key);
}

bool isCallable(JSContextRef ctx, JSValueRef value)
{
    if (!JSValueIsObject(ctx, value))
    {
        return false;
    }
    return JSObjectIsFunction(ctx, JSValueToObject(ctx, value, nullptr));
}

void throwError(JSContextRef ctx, JSValueRef *exception, const std::string &message)
{
    if (!exception)
    {
        return;
    }
    JSValueRef argument = makeString(ctx, message);
    *exception = JSObjectMakeError(ctx, 1, &argument, nullptr);
}

bool toInt64(double value, int64_t &out)
{
    // Range check first: converting an out-of-range double to an integer is undefined.
    if (!std::isfinite(value) || value < -9.2e18 || value > 9.2e18)
    {
        return false;
    }
    out = static_cast<int64_t>(value);
    return true;
}

bool toUint32(double value, uint32_t &out)
{
    if (!std::isfinite(value) || value < 0.0 || value > 4294967295.0)
    {
        return false;
    }
    out = static_cast<uint32_t>(value);
    return true;
}

bool toInt(double value, int &out)
{
    if (!std::isfinite(value) || value < -2147483648.0 || value > 2147483647.0)
    {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

void logWarning(const std::string &message)
{
    std::fprintf(stdout, "[RialtoPlayerJS] WARN: %s\n", message.c_str());
}

} // namespace jsbinding
} // namespace rialto_refplayer
