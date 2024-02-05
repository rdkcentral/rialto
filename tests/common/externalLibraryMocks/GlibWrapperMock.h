/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIREBOLT_RIALTO_WRAPPERS_GLIB_WRAPPER_MOCK_H_
#define FIREBOLT_RIALTO_WRAPPERS_GLIB_WRAPPER_MOCK_H_

#include "IGlibWrapper.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::wrappers
{
class GlibWrapperMock : public IGlibWrapper
{
public:
    GlibWrapperMock() = default;
    virtual ~GlibWrapperMock() = default;

    MOCK_METHOD(gpointer, gTypeClassRef, (GType type), (override));
    MOCK_METHOD(GType, gTypeFromName, (const gchar *name), (override));
    MOCK_METHOD(GFlagsValue *, gFlagsGetValueByNick, (GFlagsClass * flags_class, const gchar *nick), (override));
    MOCK_METHOD(void, gObjectUnref, (gpointer object), (override));
    MOCK_METHOD(gulong, gSignalConnect,
                (gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data), (override));
    MOCK_METHOD(void, gSignalHandlerDisconnect, (GObject * instance, gulong handler_id), (const, override));
    MOCK_METHOD(guint, gTimeoutAdd, (guint interval, GSourceFunc function, gpointer data), (override));
    MOCK_METHOD(gboolean, gSourceRemove, (guint tag));
    MOCK_METHOD(void, gFree, (gpointer mem), (const, override));

    // Cannot mock variadic functions
    void gObjectSet(gpointer object, const gchar *first_property_name, ...) override
    {
        va_list args;
        const gchar *kProperty = first_property_name;

        va_start(args, first_property_name);

        while (NULL != kProperty)
        {
            gObjectSetStub(object, kProperty);

            // Get the next propery, ignore the values
            va_arg(args, void *);
            kProperty = va_arg(args, const gchar *);
        }

        va_end(args);
    };
    MOCK_METHOD(void, gObjectSetStub, (gpointer object, const gchar *first_property_name));
    void gObjectGet(gpointer object, const gchar *first_property_name, ...) override
    {
        va_list args;
        const gchar *kProperty = first_property_name;

        va_start(args, first_property_name);

        while (NULL != kProperty)
        {
            void *element = va_arg(args, void *);
            gObjectGetStub(object, kProperty, element);
            kProperty = va_arg(args, const gchar *);
        }

        va_end(args);
    }
    MOCK_METHOD(void, gObjectGetStub, (gpointer object, const gchar *first_property_name, void *element));

    gchar *gStrdupPrintf(const gchar *format, ...) override { return gStrdupPrintfStub(format); };
    MOCK_METHOD(gchar *, gStrdupPrintfStub, (const gchar *format));

    MOCK_METHOD(GParamSpec *, gObjectClassFindProperty, (GObjectClass *, const gchar *), (override));
    MOCK_METHOD(gboolean, gStrHasPrefix, (const gchar *, const gchar *), (override));
    MOCK_METHOD(guint *, gSignalListIds, (GType itype, guint *n_ids), (const, override));
    MOCK_METHOD(void, gSignalQuery, (guint signal_id, GSignalQuery *query), (const, override));
    MOCK_METHOD(GType, gTypeParent, (GType type), (const, override));
    MOCK_METHOD(GType, gObjectType, (gpointer object), (const, override));
    MOCK_METHOD(gpointer, gMalloc, (gsize n_bytes), (const, override));
    MOCK_METHOD(gpointer, gMemdup, (gconstpointer mem, guint byte_size), (const, override));
    MOCK_METHOD(gboolean, gOnceInitEnter, (gsize * location), (const, override));
    MOCK_METHOD(void, gOnceInitLeave, (gsize * location, gsize result), (const, override));
    MOCK_METHOD(gchar *, gStrrstr, (const gchar *haystack, const gchar *needle), (const, override));
    MOCK_METHOD(void, gErrorFree, (GError * error), (const, override));
    MOCK_METHOD(const gchar *, gTypeName, (GType type), (const, override));
    MOCK_METHOD(int, gStrcmp0, (const char *str1, const char *str2), (const, override));
    MOCK_METHOD(gpointer, gValueGetObject, (const GValue *value), (const, override));
    MOCK_METHOD(void, gValueUnset, (GValue * value), (const, override));
    MOCK_METHOD(GError *, gErrorNewLiteral, (GQuark domain, gint code, const gchar *message), (const, override));
    GObject* gObjectNew(GType object_type, const gchar* first_property_name, ...) override
    {
        GObject* obj = nullptr;
        va_list args;
        const gchar *kProperty = first_property_name;

        va_start(args, first_property_name);

        while (NULL != kProperty)
        {
            void *element = va_arg(args, void *);
            obj = gObjectNewStub(object_type, kProperty, element);
            kProperty = va_arg(args, const gchar *);
        }

        va_end(args);

        return obj;
    }
    MOCK_METHOD(GObject*, gObjectNewStub, (GType object_type, const gchar* first_property_name, void *element));
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_GLIB_WRAPPER_MOCK_H_
