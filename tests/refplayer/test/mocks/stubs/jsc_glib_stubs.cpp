// Trivial definitions of the JavaScriptCore and GLib mock declarations. These exist only
// so the JS binding can be linked into a shared object for the export check. They have no
// behaviour and must not be used to run anything.

#include <JavaScriptCore/JavaScript.h>
#include <glib.h>

const JSClassDefinition kJSClassDefinitionEmpty = {};

JSClassRef JSClassCreate(const JSClassDefinition *) { return nullptr; }
void JSClassRelease(JSClassRef) {}

JSObjectRef JSObjectMake(JSContextRef, JSClassRef, void *) { return nullptr; }
JSObjectRef JSObjectMakeConstructor(JSContextRef, JSClassRef, JSObjectCallAsConstructorCallback) { return nullptr; }
JSObjectRef JSObjectMakeArray(JSContextRef, size_t, const JSValueRef[], JSValueRef *) { return nullptr; }
JSObjectRef JSObjectMakeError(JSContextRef, size_t, const JSValueRef[], JSValueRef *) { return nullptr; }
void *JSObjectGetPrivate(JSObjectRef) { return nullptr; }
bool JSObjectSetPrivate(JSObjectRef, void *) { return false; }
JSValueRef JSObjectCallAsFunction(JSContextRef, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef *) { return nullptr; }
bool JSObjectIsFunction(JSContextRef, JSObjectRef) { return false; }
JSValueRef JSObjectGetProperty(JSContextRef, JSObjectRef, JSStringRef, JSValueRef *) { return nullptr; }
void JSObjectSetProperty(JSContextRef, JSObjectRef, JSStringRef, JSValueRef, JSPropertyAttributes, JSValueRef *) {}

JSObjectRef JSContextGetGlobalObject(JSContextRef) { return nullptr; }
JSGlobalContextRef JSContextGetGlobalContext(JSContextRef) { return nullptr; }

void JSValueProtect(JSContextRef, JSValueRef) {}
void JSValueUnprotect(JSContextRef, JSValueRef) {}

JSValueRef JSValueMakeNumber(JSContextRef, double) { return nullptr; }
JSValueRef JSValueMakeBoolean(JSContextRef, bool) { return nullptr; }
JSValueRef JSValueMakeNull(JSContextRef) { return nullptr; }
JSValueRef JSValueMakeUndefined(JSContextRef) { return nullptr; }
JSValueRef JSValueMakeString(JSContextRef, JSStringRef) { return nullptr; }
double JSValueToNumber(JSContextRef, JSValueRef, JSValueRef *) { return 0.0; }
bool JSValueToBoolean(JSContextRef, JSValueRef) { return false; }
JSObjectRef JSValueToObject(JSContextRef, JSValueRef, JSValueRef *) { return nullptr; }
JSStringRef JSValueToStringCopy(JSContextRef, JSValueRef, JSValueRef *) { return nullptr; }
bool JSValueIsUndefined(JSContextRef, JSValueRef) { return false; }
bool JSValueIsNull(JSContextRef, JSValueRef) { return false; }
bool JSValueIsBoolean(JSContextRef, JSValueRef) { return false; }
bool JSValueIsNumber(JSContextRef, JSValueRef) { return false; }
bool JSValueIsString(JSContextRef, JSValueRef) { return false; }
bool JSValueIsObject(JSContextRef, JSValueRef) { return false; }
bool JSValueIsObjectOfClass(JSContextRef, JSValueRef, JSClassRef) { return false; }

JSStringRef JSStringCreateWithUTF8CString(const char *) { return nullptr; }
void JSStringRelease(JSStringRef) {}
size_t JSStringGetMaximumUTF8CStringSize(JSStringRef) { return 0; }
size_t JSStringGetUTF8CString(JSStringRef, char *, size_t) { return 0; }

guint g_idle_add_full(gint, GSourceFunc, gpointer, GDestroyNotify) { return 0; }
gboolean g_source_remove(guint) { return FALSE; }
