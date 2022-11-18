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

#ifndef FIREBOLT_RIALTO_SERVER_GLIB_WRAPPER_H_
#define FIREBOLT_RIALTO_SERVER_GLIB_WRAPPER_H_

#include "IGlibWrapper.h"
#include <memory>

namespace firebolt::rialto::server
{
/**
 * @brief IGlibWrapper factory class definition.
 */
class GlibWrapperFactory : public IGlibWrapperFactory
{
public:
    /**
     * @brief Weak pointer to the singleton factroy object.
     */
    static std::weak_ptr<IGlibWrapperFactory> m_factory;

    /**
     * @brief Weak pointer to the singleton object.
     */
    static std::weak_ptr<IGlibWrapper> m_gstWrapper;

    std::shared_ptr<IGlibWrapper> getGlibWrapper() override;
};

/**
 * @brief The definition of the GlibWrapper.
 */
class GlibWrapper : public IGlibWrapper
{
public:
    /**
     * @brief The constructor.
     */
    GlibWrapper() {}

    /**
     * @brief Virtual destructor.
     */
    virtual ~GlibWrapper() {}

    void gObjectSet(gpointer object, const gchar *first_property_name, ...) override;

    void gObjectGet(gpointer object, const gchar *first_property_name, ...) override;

    GParamSpec *gObjectClassFindProperty(GObjectClass *oclass, const gchar *property_name) override;

    gpointer gTypeClassRef(GType type) override { return g_type_class_ref(type); }

    GType gTypeFromName(const gchar *name) override { return g_type_from_name(name); }

    GFlagsValue *gFlagsGetValueByNick(GFlagsClass *flags_class, const gchar *nick) override
    {
        return g_flags_get_value_by_nick(flags_class, nick);
    }

    void gObjectUnref(gpointer object) override { g_object_unref(object); }

    void gSignalConnect(gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data) override
    {
        g_signal_connect(instance, detailed_signal, c_handler, data);
    }

    guint gTimeoutAdd(guint interval, GSourceFunc function, gpointer data) override
    {
        return g_timeout_add(interval, function, data);
    }

    gboolean gSourceRemove(guint tag) override { return g_source_remove(tag); }

    gchar *gStrdupPrintf(const gchar *format, ...) override;

    gboolean gStrHasPrefix(const gchar *str, const gchar *prefix) override;

    void gFree(gpointer mem) const override { g_free(mem); }

    guint *gSignalListIds(GType itype, guint *n_ids) const override { return g_signal_list_ids(itype, n_ids); }

    void gSignalQuery(guint signal_id, GSignalQuery *query) const override { g_signal_query(signal_id, query); }

    GType gTypeParent(GType type) const override { return g_type_parent(type); }

    GType gObjectType(gpointer object) const override { return G_OBJECT_TYPE(object); }

    gpointer gMalloc(gsize n_bytes) const override { return g_malloc(n_bytes); }

    gpointer gMemdup(gconstpointer mem, guint  byte_size) const override { return g_memdup(mem, byte_size); }
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GLIB_WRAPPER_H_
