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

#include "GlibWrapperMock.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::server
{
GlibWrapperMock::GlibWrapperMock(){};
GlibWrapperMock::~GlibWrapperMock(){};

void GlibWrapperMock::gObjectSet(gpointer object, const gchar *first_property_name, ...)
{
    va_list args;
    const gchar *property = first_property_name;

    va_start(args, first_property_name);

    while (NULL != property)
    {
        gObjectSetStub(object, property);

        // Get the next propery, ignore the values
        va_arg(args, void *);
        property = va_arg(args, const gchar *);
    }

    va_end(args);
}

void GlibWrapperMock::gObjectGet(gpointer object, const gchar *first_property_name, ...)
{
    va_list args;
    const gchar *property = first_property_name;

    va_start(args, first_property_name);

    while (NULL != property)
    {
        void *element = va_arg(args, void *);
        gObjectGetStub(object, property, element);
        property = va_arg(args, const gchar *);
    }

    va_end(args);
}

gchar *GlibWrapperMock::gStrdupPrintf(const gchar *format, ...)
{
    return gStrdupPrintfStub(format);
}
} // namespace firebolt::rialto::server
