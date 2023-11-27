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

#include "GlibWrapper.h"

namespace firebolt::rialto::wrappers
{
std::shared_ptr<IGlibWrapper> GlibWrapperFactory::getGlibWrapper()
{
    static std::shared_ptr<IGlibWrapper> gstWrapper{};
    if (!gstWrapper)
        try
        {
            gstWrapper = std::make_shared<GlibWrapper>();
        }
        catch (const std::exception &)
        {
        }

    return gstWrapper;
}

void GlibWrapper::gObjectSet(gpointer object, const gchar *first_property_name, ...)
{
    va_list vl;
    va_start(vl, first_property_name);
    g_object_set_valist(static_cast<GObject *>(object), first_property_name, vl);
    va_end(vl);
}

void GlibWrapper::gObjectGet(gpointer object, const gchar *first_property_name, ...)
{
    va_list vl;
    va_start(vl, first_property_name);
    g_object_get_valist(static_cast<GObject *>(object), first_property_name, vl);
    va_end(vl);
}

GParamSpec *GlibWrapper::gObjectClassFindProperty(GObjectClass *oclass, const gchar *property_name)
{
    return g_object_class_find_property(oclass, property_name);
}

gchar *GlibWrapper::gStrdupPrintf(const gchar *format, ...)
{
    gchar *str = NULL;
    va_list vl;
    va_start(vl, format);
    str = g_strdup_vprintf(format, vl);
    va_end(vl);
    return str;
}

gboolean GlibWrapper::gStrHasPrefix(const gchar *str, const gchar *prefix)
{
    return g_str_has_prefix(str, prefix);
}

}; // namespace firebolt::rialto::wrappers
