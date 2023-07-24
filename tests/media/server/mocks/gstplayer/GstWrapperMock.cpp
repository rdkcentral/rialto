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

#include "GstWrapperMock.h"

namespace firebolt::rialto::server
{
GstWrapperMock::GstWrapperMock(){};
GstWrapperMock::~GstWrapperMock(){};

GstCaps *GstWrapperMock::gstCapsNewSimple(const char *media_type, const char *fieldname, ...) const
{
    va_list args;
    const gchar *property = fieldname;

    va_start(args, fieldname);

    GType intType = va_arg(args, GType);
    int intValue = va_arg(args, int);
    GstCaps *result = gstCapsNewSimpleIntStub(media_type, property, intType, intValue);

    va_end(args);

    return result;
}

void GstWrapperMock::gstCapsSetSimple(GstCaps *caps, const gchar *field, ...) const
{
    va_list args;
    const gchar *property = field;

    va_start(args, field);

    while (NULL != property)
    {
        GType type = va_arg(args, GType);
        if (g_type_is_a(type, G_TYPE_INT))
        {
            int intValue = va_arg(args, int);
            gstCapsSetSimpleIntStub(caps, property, type, intValue);
        }
        else if (g_type_is_a(type, G_TYPE_STRING))
        {
            const char *val = va_arg(args, const char *);
            gstCapsSetSimpleStringStub(caps, property, type, val);
        }
        else if (g_type_is_a(type, GST_TYPE_BUFFER))
        {
            GstBuffer *buf = va_arg(args, GstBuffer *);
            gstCapsSetSimpleBufferStub(caps, property, type, buf);
        }
        else if (g_type_is_a(type, G_TYPE_BOOLEAN))
        {
            gboolean val = va_arg(args, gboolean);
            gstCapsSetSimpleBooleanStub(caps, property, type, val);
        }
        else if (g_type_is_a(type, G_TYPE_UINT))
        {
            unsigned val = va_arg(args, unsigned);
            gstCapsSetSimpleUintStub(caps, property, type, val);
        }
        else if (g_type_is_a(type, GST_TYPE_BITMASK))
        {
            uint64_t val = va_arg(args, uint64_t);
            gstCapsSetSimpleBitMaskStub(caps, property, type, val);
        }
        else if (g_type_is_a(type, GST_TYPE_FRACTION))
        {
            int val1 = va_arg(args, int);
            int val2 = va_arg(args, int);
            gstCapsSetSimpleFractionStub(caps, property, type, val1, val2);
        }
        property = va_arg(args, const gchar *);
    }

    va_end(args);
}

GstStructure *GstWrapperMock::gstStructureNew(const gchar *name, const gchar *firstfield, ...) const
{
    GstStructure *structure{nullptr};
    va_list args;
    const gchar *property = firstfield;

    va_start(args, firstfield);

    while (NULL != property)
    {
        GType valueType = va_arg(args, GType);
        if (g_type_is_a(valueType, G_TYPE_DOUBLE))
        {
            double value = va_arg(args, double);
            structure = gstStructureNewDoubleStub(name, property, valueType, value);
        }
        else if (g_type_is_a(valueType, G_TYPE_BOOLEAN))
        {
            gboolean value = va_arg(args, gboolean);
            structure = gstStructureNewBoolStub(name, property, valueType, value);
        }
        else if (g_type_is_a(valueType, GST_TYPE_BUFFER))
        {
            GstBuffer *value = va_arg(args, GstBuffer *);
            structure = gstStructureNewBufferStub(name, property, valueType, value);
        }
        else if (g_type_is_a(valueType, G_TYPE_UINT))
        {
            uint32_t value = va_arg(args, guint32);
            structure = gstStructureNewUintStub(name, property, valueType, value);
        }
        else if (g_type_is_a(valueType, G_TYPE_STRING))
        {
            const char *value = va_arg(args, const char *);
            structure = gstStructureNewStringStub(name, property, valueType, value);
        }
        property = va_arg(args, const gchar *);
    }

    va_end(args);
    return structure;
}

void GstWrapperMock::gstStructureSet(GstStructure *structure, const gchar *firstname, ...) const
{
    va_list args;
    const gchar *field = firstname;

    va_start(args, firstname);

    while (NULL != field)
    {
        GType type = va_arg(args, GType);
        if (g_type_is_a(type, G_TYPE_STRING))
        {
            const char *val = va_arg(args, const char *);
            gstStructureSetStringStub(structure, field, type, val);
        }
        else if (g_type_is_a(type, G_TYPE_UINT))
        {
            unsigned val = va_arg(args, unsigned);
            gstStructureSetUintStub(structure, field, type, val);
        }
        field = va_arg(args, const gchar *);
    }

    va_end(args);
}
} // namespace firebolt::rialto::server
