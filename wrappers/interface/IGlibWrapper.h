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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_GLIB_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_GLIB_WRAPPER_H_

#include <gst/gst.h>
#include <memory>
#include <stdint.h>
#include <string>

namespace firebolt::rialto::wrappers
{
class IGlibWrapper;

/**
 * @brief IGlibWrapper factory class, for the IGlibWrapper singleton object.
 */
class IGlibWrapperFactory
{
public:
    IGlibWrapperFactory() = default;
    virtual ~IGlibWrapperFactory() = default;

    /**
     * @brief Gets the IGlibWrapperFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IGlibWrapperFactory> getFactory();

    /**
     * @brief Gets a IGlibWrapper singleton object.
     *
     * @retval the wrapper instance or null on error.
     */
    virtual std::shared_ptr<IGlibWrapper> getGlibWrapper() = 0;
};

class IGlibWrapper
{
public:
    IGlibWrapper() = default;
    virtual ~IGlibWrapper() = default;

    IGlibWrapper(const IGlibWrapper &) = delete;
    IGlibWrapper &operator=(const IGlibWrapper &) = delete;
    IGlibWrapper(IGlibWrapper &&) = delete;
    IGlibWrapper &operator=(IGlibWrapper &&) = delete;

    /**
     * @brief Set the property on an object.
     *
     * @param[in] object                : Pointer to the object.
     * @param[in] first_property_name   : Name of the first property.
     * @param[in] ...                   : The value of the first propery, followed subsequenty by any other name value pairs.
     */
    virtual void gObjectSet(gpointer object, const gchar *first_property_name, ...) = 0;

    /**
     * @brief Gets properties of an object.
     *
     * @param[in] object                : Pointer to the object.
     * @param[in] first_property_name   : Name of the first property to get.
     * @param[in] ...                   : Return location for the first property, followed optionally by more
     * name/return location pairs
     */
    virtual void gObjectGet(gpointer object, const gchar *first_property_name, ...) = 0;

    /**
     * @brief Looks up the GParamSpec for a property of a class.
     *
     * @param[in] oclass        : a GObjectClass
     * @param[in] property_name : the name of the property to look up
     *
     * @retval the GParamSpec for the property, or NULL if the class doesn't have a property of that name.
     */
    virtual GParamSpec *gObjectClassFindProperty(GObjectClass *oclass, const gchar *property_name) = 0;

    /**
     * @brief Increments the reference count of the class. Creates the class if it does not exist.
     *
     * @param[in] type  : Type of class to increment.
     *
     * @retval Pointer to the class structure.
     */
    virtual gpointer gTypeClassRef(GType type) = 0;

    /**
     * @brief Gets the type from the name.
     *
     * @param[in] name  : The name of the type to find
     *
     * @retval Type or 0 on error.
     */
    virtual GType gTypeFromName(const gchar *name) = 0;

    /**
     * @brief Get the flag from the nickname.
     *
     * @param[in] flags_class   : The flag class.
     * @param[in] nick          : Nickname of the flag.
     *
     * @retval The value of the flag or NULL on error.
     */
    virtual GFlagsValue *gFlagsGetValueByNick(GFlagsClass *flags_class, const gchar *nick) = 0;

    /**
     * @brief Decrement reference count on object.
     *
     * @param[in] object   : Object to decrement.
     */
    virtual void gObjectUnref(gpointer object) = 0;

    /**
     * @brief Register for a gstreamer signal.
     *
     * @param[in] instance          : The instance to register.
     * @param[in] detailed_signal   : The signal in the form of a string.
     * @param[in] c_handler         : The callback.
     * @param[in] data              : Data to be passed to the signal.
     *
     * @retval the handler ID, of type gulong (always greater than 0 for successful connections)
     */
    virtual gulong gSignalConnect(gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data) = 0;

    /**
     * @brief Similar to sprintf. Returning string should be freed using gFree.
     *
     * @param[in] format    : Format string.
     * @param[in] ...       : Values to populate the format string.
     *
     * @retval Allocated string.
     */
    virtual gchar *gStrdupPrintf(const gchar *format, ...) = 0;

    /**
     * @brief Looks whether the string str begins with prefix.
     *
     * @param[in] str    : A nul-terminated string.
     * @param[in] prefix : The nul-terminated prefix to look for.
     *
     * @retval TRUE if str begins with prefix, FALSE otherwise.
     */
    virtual gboolean gStrHasPrefix(const gchar *str, const gchar *prefix) = 0;

    /**
     * @brief Free memory.
     *
     * @param[in] mem  : Memory to free.
     */
    virtual void gFree(gpointer mem) const = 0;

    /**
     * @brief Lists the signals by id that a certain instance or interface type created. Further information about the
     *        signals can be acquired through g_signal_query().
     *
     * @param[in] itype  : Instance or interface type.
     * @param[in] n_ids  : Location to store the number of signal ids for itype.
     *
     * @retval: Newly allocated array of signal IDs.
     */
    virtual guint *gSignalListIds(GType itype, guint *n_ids) const = 0;

    /**
     * @brief Queries the signal system for in-depth information about a specific signal. This function will fill in a
     *        user-provided structure to hold signal-specific information. If an invalid signal id is passed in, the
     *        signal_id member of the GSignalQuery is 0. All members filled into the GSignalQuery structure should be
     *        considered constant and have to be left untouched.
     *
     * @param[in] signal_id : The signal id of the signal to query information for.
     * @param[in] query     : A user provided structure that is filled in with constant values upon success.
     *
     * @retval: Newly allocated array of signal IDs.
     */
    virtual void gSignalQuery(guint signal_id, GSignalQuery *query) const = 0;

    virtual GType gTypeParent(GType type) const = 0;

    /**
     * @brief Returns GType of the object
     *
     * @param[in] object                : Pointer to the object.
     *
     * @retval GType of the object
     */
    virtual GType gObjectType(gpointer object) const = 0;

    /**
     * @brief Allocates n_bytes bytes of memory. If n_bytes is 0 it returns NULL.
     *
     * @param[in] n_bytes : The number of bytes to allocate.
     *
     * @retval A pointer to the allocated memory.
     */
    virtual gpointer gMalloc(gsize n_bytes) const = 0;

    /**
     * @brief Allocates byte_size bytes of memory, and copies byte_size bytes into it from mem.
     *
     * @param[in] mem : The memory to copy from
     * @param[in] byte_size : The number of bytes to copy.
     *
     * @retval A pointer to the newly-allocated copy of the memory or null in case of failure
     */
    virtual gpointer gMemdup(gconstpointer mem, guint byte_size) const = 0;

    /**
     * @brief Called when executing a critical initalisation section. Ensures that the init is
     *        only performed once and all other threads wait for the execution.
     *
     * @param[in] location : Location of initalised variable.
     *
     * @retval If the initalisation section can be entered.
     */
    virtual gboolean gOnceInitEnter(gsize *location) const = 0;

    /**
     * @brief Sets the init variable to the initalised value and releases threads blocking on
     *        gOnceInitEnter.
     *
     * @param[in] location : Location of initalised variable.
     * @param[in] result : New value for the location.
     *
     * @retval A pointer to the newly-allocated copy of the memory or null in case of failure
     */
    virtual void gOnceInitLeave(gsize *location, gsize result) const = 0;

    /**
     * @brief Searches the string haystack for the last occurrence of the string needle.
     *
     * @param[in] haystack : A null-terminated string.
     * @param[in] needle   : The null-terminated string to search for.
     *
     * @retval A pointer to the found occurrence, or NULL if not found.
     */
    virtual gchar *gStrrstr(const gchar *haystack, const gchar *needle) const = 0;

    /**
     * @brief Frees a GError and its resources.
     *
     * @param[in] error : GError to free.
     */
    virtual void gErrorFree(GError *error) const = 0;

    /**
     * @brief Gets the name of the given type.
     *
     * @param[in] type : type id.
     *
     * @retval type name or NULL.
     */
    virtual const gchar *gTypeName(GType type) const = 0;

    /**
     * @brief Compare strings. NULL strings are handled, comparing two NULL
     *        strings returns 0.
     *
     * @param[in] str1 : First string.
     * @param[in] str2 : Second string.
     *
     * @retval type name or NULL.
     */
    virtual int gStrcmp0(const char *str1, const char *str2) const = 0;

    /**
     * @brief Get the object from the value.
     *
     * @param[in] value : The value to get.
     *
     * @retval < 0 or > 0 if str1 < or > str2, 0 if they are equal.
     */
    virtual gpointer gValueGetObject(const GValue *value) const = 0;

    /**
     * @brief Clears the value and unsets the type.
     *
     * @param[in] value : Value to unset.
     */
    virtual void gValueUnset(GValue *value) const = 0;

    /**
     * @brief Create a new GError.
     *
     * @param[in] domain    : Domain of the error.
     * @param[in] code      : Error code.
     * @param[in] message   : Error message.
     */
    virtual GError *gErrorNewLiteral(GQuark domain, gint code, const gchar *message) const = 0;
};

}; // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_GLIB_WRAPPER_H_
