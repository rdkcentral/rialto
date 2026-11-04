// Compile-time subset of the JavaScriptCore C API. Typedefs and signatures match
// WebKit's JSC headers. Used only to compile and link the JS binding on a machine
// without JavaScriptCore. Not a runtime implementation.
#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct OpaqueJSContext *JSContextRef;
typedef struct OpaqueJSContext *JSGlobalContextRef;
typedef struct OpaqueJSClass *JSClassRef;
typedef const struct OpaqueJSValue *JSValueRef;
typedef struct OpaqueJSValue *JSObjectRef;
typedef struct OpaqueJSString *JSStringRef;
typedef struct OpaqueJSPropertyNameAccumulator *JSPropertyNameAccumulatorRef;

typedef enum {
    kJSTypeUndefined,
    kJSTypeNull,
    kJSTypeBoolean,
    kJSTypeNumber,
    kJSTypeString,
    kJSTypeObject,
    kJSTypeSymbol
} JSType;

enum {
    kJSPropertyAttributeNone = 0,
    kJSPropertyAttributeReadOnly = 1 << 1,
    kJSPropertyAttributeDontEnum = 1 << 2,
    kJSPropertyAttributeDontDelete = 1 << 3
};
typedef unsigned JSPropertyAttributes;

enum {
    kJSClassAttributeNone = 0,
    kJSClassAttributeNoAutomaticPrototype = 1 << 1
};
typedef unsigned JSClassAttributes;

typedef void (*JSObjectInitializeCallback)(JSContextRef ctx, JSObjectRef object);
typedef void (*JSObjectFinalizeCallback)(JSObjectRef object);
typedef bool (*JSObjectHasPropertyCallback)(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName);
typedef JSValueRef (*JSObjectGetPropertyCallback)(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef *exception);
typedef bool (*JSObjectSetPropertyCallback)(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef *exception);
typedef bool (*JSObjectDeletePropertyCallback)(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef *exception);
typedef void (*JSObjectGetPropertyNamesCallback)(JSContextRef ctx, JSObjectRef object, JSPropertyNameAccumulatorRef propertyNames);
typedef JSValueRef (*JSObjectCallAsFunctionCallback)(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
typedef JSObjectRef (*JSObjectCallAsConstructorCallback)(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
typedef bool (*JSObjectHasInstanceCallback)(JSContextRef ctx, JSObjectRef constructor, JSValueRef possibleInstance, JSValueRef *exception);
typedef JSValueRef (*JSObjectConvertToTypeCallback)(JSContextRef ctx, JSObjectRef object, JSType type, JSValueRef *exception);

typedef struct JSStaticValue {
    const char *name;
    JSObjectGetPropertyCallback getProperty;
    JSObjectSetPropertyCallback setProperty;
    JSPropertyAttributes attributes;
} JSStaticValue;

typedef struct JSStaticFunction {
    const char *name;
    JSObjectCallAsFunctionCallback callAsFunction;
    JSPropertyAttributes attributes;
} JSStaticFunction;

typedef struct JSClassDefinition {
    int version;
    JSClassAttributes attributes;
    const char *className;
    JSClassRef parentClass;
    const JSStaticValue *staticValues;
    const JSStaticFunction *staticFunctions;
    JSObjectInitializeCallback initialize;
    JSObjectFinalizeCallback finalize;
    JSObjectHasPropertyCallback hasProperty;
    JSObjectGetPropertyCallback getProperty;
    JSObjectSetPropertyCallback setProperty;
    JSObjectDeletePropertyCallback deleteProperty;
    JSObjectGetPropertyNamesCallback getPropertyNames;
    JSObjectCallAsFunctionCallback callAsFunction;
    JSObjectCallAsConstructorCallback callAsConstructor;
    JSObjectHasInstanceCallback hasInstance;
    JSObjectConvertToTypeCallback convertToType;
} JSClassDefinition;

extern const JSClassDefinition kJSClassDefinitionEmpty;

// Classes
JSClassRef JSClassCreate(const JSClassDefinition *definition);
void JSClassRelease(JSClassRef jsClass);

// Objects
JSObjectRef JSObjectMake(JSContextRef ctx, JSClassRef jsClass, void *data);
JSObjectRef JSObjectMakeConstructor(JSContextRef ctx, JSClassRef jsClass, JSObjectCallAsConstructorCallback callAsConstructor);
JSObjectRef JSObjectMakeArray(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
JSObjectRef JSObjectMakeError(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
void *JSObjectGetPrivate(JSObjectRef object);
bool JSObjectSetPrivate(JSObjectRef object, void *data);
JSValueRef JSObjectCallAsFunction(JSContextRef ctx, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
bool JSObjectIsFunction(JSContextRef ctx, JSObjectRef object);
JSValueRef JSObjectGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef *exception);
void JSObjectSetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSPropertyAttributes attributes, JSValueRef *exception);

// Context
JSObjectRef JSContextGetGlobalObject(JSContextRef ctx);
JSGlobalContextRef JSContextGetGlobalContext(JSContextRef ctx);

// GC roots
void JSValueProtect(JSContextRef ctx, JSValueRef value);
void JSValueUnprotect(JSContextRef ctx, JSValueRef value);

// Values
JSValueRef JSValueMakeNumber(JSContextRef ctx, double number);
JSValueRef JSValueMakeBoolean(JSContextRef ctx, bool boolean);
JSValueRef JSValueMakeNull(JSContextRef ctx);
JSValueRef JSValueMakeUndefined(JSContextRef ctx);
JSValueRef JSValueMakeString(JSContextRef ctx, JSStringRef string);
double JSValueToNumber(JSContextRef ctx, JSValueRef value, JSValueRef *exception);
bool JSValueToBoolean(JSContextRef ctx, JSValueRef value);
JSObjectRef JSValueToObject(JSContextRef ctx, JSValueRef value, JSValueRef *exception);
JSStringRef JSValueToStringCopy(JSContextRef ctx, JSValueRef value, JSValueRef *exception);
bool JSValueIsUndefined(JSContextRef ctx, JSValueRef value);
bool JSValueIsNull(JSContextRef ctx, JSValueRef value);
bool JSValueIsBoolean(JSContextRef ctx, JSValueRef value);
bool JSValueIsNumber(JSContextRef ctx, JSValueRef value);
bool JSValueIsString(JSContextRef ctx, JSValueRef value);
bool JSValueIsObject(JSContextRef ctx, JSValueRef value);
bool JSValueIsObjectOfClass(JSContextRef ctx, JSValueRef value, JSClassRef jsClass);

// Strings
JSStringRef JSStringCreateWithUTF8CString(const char *string);
void JSStringRelease(JSStringRef string);
size_t JSStringGetMaximumUTF8CStringSize(JSStringRef string);
size_t JSStringGetUTF8CString(JSStringRef string, char *buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif
