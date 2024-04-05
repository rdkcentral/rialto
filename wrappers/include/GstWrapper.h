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

#ifndef FIREBOLT_RIALTO_WRAPPERS_GST_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_GST_WRAPPER_H_

#include "IGstWrapper.h"
#include <cassert>
#include <gst/pbutils/pbutils.h>
#include <memory>
#include <mutex>
#include <string>

namespace firebolt::rialto::wrappers
{
/**
 * @brief IGstWrapper factory class definition.
 */
class GstWrapperFactory : public IGstWrapperFactory
{
public:
    /**
     * @brief Weak pointer to the singleton factroy object.
     */
    static std::weak_ptr<IGstWrapperFactory> m_factory;

    /**
     * @brief Weak pointer to the singleton object.
     */
    static std::weak_ptr<IGstWrapper> m_gstWrapper;

    /**
     * @brief Mutex protection for creation of the Control object.
     */
    static std::mutex m_creationMutex;

    std::shared_ptr<IGstWrapper> getGstWrapper() override;
};

/**
 * @brief The definition of the GstWrapper.
 */
class GstWrapper : public IGstWrapper
{
public:
    /**
     * @brief The constructor.
     */
    GstWrapper() {}

    /**
     * @brief Virtual destructor.
     */
    virtual ~GstWrapper() {}

    void gstInit(int *argc, char ***argv) override { gst_init(argc, argv); }

    GstPlugin *gstRegistryFindPlugin(GstRegistry *registry, const gchar *name) override
    {
        return gst_registry_find_plugin(registry, name);
    }

    void gstRegistryRemovePlugin(GstRegistry *registry, GstPlugin *plugin) override
    {
        return gst_registry_remove_plugin(registry, plugin);
    }

    void gstObjectUnref(gpointer object) override { gst_object_unref(object); }

    GstRegistry *gstRegistryGet() override { return gst_registry_get(); }

    GstElementFactory *gstElementFactoryFind(const gchar *name) override { return gst_element_factory_find(name); }

    gboolean gstElementRegister(GstPlugin *plugin, const gchar *name, guint rank, GType type) override
    {
        return gst_element_register(plugin, name, rank, type);
    }

    GstElement *gstElementFactoryMake(const gchar *factoryname, const gchar *name) override
    {
        return gst_element_factory_make(factoryname, name);
    }

    GstElement *gstBinGetByName(GstBin *bin, const gchar *name) override { return gst_bin_get_by_name(bin, name); }

    gpointer gstObjectRef(gpointer object) override { return gst_object_ref(object); }

    GstObject *gstObjectParent(gpointer object) const override { return GST_OBJECT_PARENT(object); }

    GstObject *gstObjectCast(gpointer object) const override { return GST_OBJECT_CAST(object); }

    GstBus *gstPipelineGetBus(GstPipeline *pipeline) override { return gst_pipeline_get_bus(pipeline); }

    guint gstBusAddWatch(GstBus *bus, GstBusFunc func, gpointer user_data) override
    {
        return gst_bus_add_watch(bus, func, user_data);
    }

    void gstMessageParseStateChanged(GstMessage *message, GstState *oldstate, GstState *newstate, GstState *pending) override
    {
        return gst_message_parse_state_changed(message, oldstate, newstate, pending);
    }

    const gchar *gstElementStateGetName(GstState state) override { return gst_element_state_get_name(state); }

    GstStateChangeReturn gstElementSetState(GstElement *element, GstState state) override
    {
        return gst_element_set_state(element, state);
    }

    GstState gstElementGetState(GstElement *element) override { return GST_STATE(element); }

    GstState gstElementGetPendingState(GstElement *element) override { return GST_STATE_PENDING(element); }

    GstObject *gstElementGetParent(const GstElement *elem) const override { return gst_element_get_parent(elem); }

    gchar *gstElementGetName(GstElement *element) const override { return gst_element_get_name(element); }

    gboolean gstElementSendEvent(GstElement *element, GstEvent *event) const override
    {
        return gst_element_send_event(element, event);
    }

    void gstAppSrcSetCallbacks(GstAppSrc *appsrc, GstAppSrcCallbacks *callbacks, gpointer userData,
                               GDestroyNotify notify) override
    {
        gst_app_src_set_callbacks(appsrc, callbacks, userData, notify);
    }

    void gstAppSrcSetMaxBytes(GstAppSrc *appsrc, guint64 max) override { gst_app_src_set_max_bytes(appsrc, max); }

    void gstAppSrcSetStreamType(GstAppSrc *appsrc, GstAppStreamType type) override
    {
        gst_app_src_set_stream_type(appsrc, type);
    }

    GstFlowReturn gstAppSrcEndOfStream(GstAppSrc *appsrc) override { return gst_app_src_end_of_stream(appsrc); }

    gboolean gstBinAdd(GstBin *bin, GstElement *element) override { return gst_bin_add(bin, element); }

    void gstBaseTransformSetInPlace(GstBaseTransform *trans, gboolean in_place) override
    {
        gst_base_transform_set_in_place(trans, in_place);
    }

    gboolean gstElementSyncStateWithParent(GstElement *element) override
    {
        return gst_element_sync_state_with_parent(element);
    }

    gboolean gstElementLink(GstElement *src, GstElement *dest) override { return gst_element_link(src, dest); }

    GstPad *gstElementGetStaticPad(GstElement *element, const gchar *name) override
    {
        return gst_element_get_static_pad(element, name);
    }

    gboolean gstElementQueryPosition(GstElement *element, GstFormat format, gint64 *cur) override
    {
        return gst_element_query_position(element, format, cur);
    }

    GstPad *gstGhostPadNew(const gchar *name, GstPad *target) override { return gst_ghost_pad_new(name, target); }

    void gstPadSetQueryFunction(GstPad *pad, GstPadQueryFunction query) override
    {
        gst_pad_set_query_function(pad, query);
    }

    gboolean gstPadSetActive(GstPad *pad, gboolean active) override { return gst_pad_set_active(pad, active); }

    gboolean gstPadSendEvent(GstPad *pad, GstEvent *event) override { return gst_pad_send_event(pad, event); }

    gboolean gstElementAddPad(GstElement *element, GstPad *pad) override { return gst_element_add_pad(element, pad); }

    gboolean gstElementSeek(GstElement *element, gdouble rate, GstFormat format, GstSeekFlags flags,
                            GstSeekType start_type, gint64 start, GstSeekType stop_type, gint64 stop) override
    {
        return gst_element_seek(element, rate, format, flags, start_type, start, stop_type, stop);
    }

    void gstElementNoMorePads(GstElement *element) override { gst_element_no_more_pads(element); }

    GstElement *gstElementFactoryCreate(GstElementFactory *factory, const gchar *name) override
    {
        return gst_element_factory_create(factory, name);
    }

    GstCaps *gstCapsFromString(const gchar *string) override { return gst_caps_from_string(string); }

    void gstAppSrcSetCaps(GstAppSrc *appsrc, const GstCaps *caps) override { gst_app_src_set_caps(appsrc, caps); }

    GstCaps *gstAppSrcGetCaps(GstAppSrc *appsrc) override { return gst_app_src_get_caps(appsrc); }

    gboolean gstCapsIsEqual(const GstCaps *caps1, const GstCaps *caps2) override
    {
        return gst_caps_is_equal(caps1, caps2);
    }

    gchar *gstCapsToString(const GstCaps *caps) override { return gst_caps_to_string(caps); }

    void gstCapsUnref(GstCaps *caps) override { gst_caps_unref(caps); }

    GstBuffer *gstBufferNew() override { return gst_buffer_new(); }

    GstBuffer *gstBufferNewAllocate(GstAllocator *allocator, gsize size, GstAllocationParams *params) override
    {
        return gst_buffer_new_allocate(allocator, size, params);
    }

    gsize gstBufferFill(GstBuffer *buffer, gsize offset, gconstpointer src, gsize size) override
    {
        return gst_buffer_fill(buffer, offset, src, size);
    }

    void gstBufferUnref(GstBuffer *buf) override { gst_buffer_unref(buf); }

    gboolean gstBufferMap(GstBuffer *buffer, GstMapInfo *info, GstMapFlags flags) override
    {
        return gst_buffer_map(buffer, info, flags);
    }

    void gstBufferUnmap(GstBuffer *buffer, GstMapInfo *info) override { gst_buffer_unmap(buffer, info); }

    void gstBusSetSyncHandler(GstBus *bus, GstBusSyncHandler func, gpointer user_data, GDestroyNotify notify) override
    {
        gst_bus_set_sync_handler(bus, func, user_data, notify);
    }

    GstFlowReturn gstAppSrcPushBuffer(GstAppSrc *appsrc, GstBuffer *buffer) override
    {
        return gst_app_src_push_buffer(appsrc, buffer);
    }

    void gstMessageUnref(GstMessage *msg) override { gst_message_unref(msg); }

    GstMessage *gstBusTimedPopFiltered(GstBus *bus, GstClockTime timeout, GstMessageType types) override
    {
        return gst_bus_timed_pop_filtered(bus, timeout, types);
    }

    void gstDebugBinToDotFileWithTs(GstBin *bin, GstDebugGraphDetails details, const gchar *file_name) override
    {
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(bin, details, file_name);
    }

    GstElementFactory *gstElementGetFactory(GstElement *element) const override
    {
        return gst_element_get_factory(element);
    }

    gboolean gstElementFactoryListIsType(GstElementFactory *factory, GstElementFactoryListType type) const override
    {
        return gst_element_factory_list_is_type(factory, type);
    }

    GstCaps *gstCapsCopy(const GstCaps *caps) const override { return gst_caps_copy(caps); }

    void gstCapsSetSimple(GstCaps *caps, const gchar *field, ...) const override;

    void gstMessageParseQos(GstMessage *message, gboolean *live, guint64 *running_time, guint64 *stream_time,
                            guint64 *timestamp, guint64 *duration) const override
    {
        gst_message_parse_qos(message, live, running_time, stream_time, timestamp, duration);
    }

    void gstMessageParseQosStats(GstMessage *message, GstFormat *format, guint64 *processed, guint64 *dropped) const override
    {
        gst_message_parse_qos_stats(message, format, processed, dropped);
    }

    const gchar *gstElementClassGetMetadata(GstElementClass *klass, const gchar *key) const override
    {
        return gst_element_class_get_metadata(klass, key);
    }

    const gchar *gstFormatGetName(GstFormat format) const override { return gst_format_get_name(format); }

    GstSegment *gstSegmentNew() const override { return gst_segment_new(); }

    void gstSegmentInit(GstSegment *segment, GstFormat format) const override { gst_segment_init(segment, format); }

    void gstSegmentFree(GstSegment *segment) const override { gst_segment_free(segment); }

    GstEvent *gstEventNewSegment(const GstSegment *segment) const override { return gst_event_new_segment(segment); }

    GstEvent *gstEventNewCustom(GstEventType type, GstStructure *structure) const override
    {
        return gst_event_new_custom(type, structure);
    };

    GstStructure *gstStructureNew(const gchar *name, const gchar *firstfield, ...) const override;

    void gstByteWriterInitWithData(GstByteWriter *writer, guint8 *data, guint size, gboolean initialized) const override
    {
        return gst_byte_writer_init_with_data(writer, data, size, initialized);
    }

    gboolean gstByteWriterPutUint16Be(GstByteWriter *writer, guint16 val) const override
    {
        return gst_byte_writer_put_uint16_be(writer, val);
    }

    gboolean gstByteWriterPutUint32Be(GstByteWriter *writer, guint32 val) const override
    {
        return gst_byte_writer_put_uint32_be(writer, val);
    }

    GstBuffer *gstBufferNewWrapped(gpointer data, gsize size) const override
    {
        return gst_buffer_new_wrapped(data, size);
    }

    GstCaps *gstCodecUtilsOpusCreateCapsFromHeader(gconstpointer data, guint size) const override
    {
#if (GLIB_CHECK_VERSION(2, 67, 3))
        GstBuffer *tmp = gst_buffer_new_wrapped(g_memdup2(data, size), size);
#else
        const gsize kByteSize = static_cast<gsize>(size);
        assert(kByteSize >= 0);
        GstBuffer *tmp = gst_buffer_new_wrapped(g_memdup(data, kByteSize), size);
#endif
        GstCaps *gst_caps = gst_codec_utils_opus_create_caps_from_header(tmp, NULL);
        gst_buffer_unref(tmp);
        return gst_caps;
    }

    gboolean gstCapsIsSubset(const GstCaps *subset, const GstCaps *superset) const override
    {
        return gst_caps_is_subset(subset, superset);
    }

    gboolean gstCapsIsStrictlyEqual(const GstCaps *caps1, const GstCaps *caps2) const override
    {
        return gst_caps_is_strictly_equal(caps1, caps2);
    }

    gboolean gstCapsCanIntersect(const GstCaps *caps1, const GstCaps *caps2) const override
    {
        return gst_caps_can_intersect(caps1, caps2);
    }

    GstCaps *gstStaticCapsGet(GstStaticCaps *staticCaps) const override { return gst_static_caps_get(staticCaps); }

    GList *gstElementFactoryListGetElements(GstElementFactoryListType type, GstRank minrank) const override
    {
        return gst_element_factory_list_get_elements(type, minrank);
    }

    const GList *gstElementFactoryGetStaticPadTemplates(GstElementFactory *factory) const override
    {
        return gst_element_factory_get_static_pad_templates(factory);
    }

    void gstPluginFeatureListFree(GList *list) const override { gst_plugin_feature_list_free(list); }

    GstCaps *gstCapsNewSimple(const char *media_type, const char *fieldname, ...) const override
    {
        /* there's no valist equivalent of gst_caps_new_simple */
        va_list varArgs;
        va_start(varArgs, fieldname);
        GstStructure *structure = gst_structure_new_valist(media_type, fieldname, varArgs);
        va_end(varArgs);

        if (structure)
        {
            GstCaps *caps = gst_caps_new_empty();
            gst_caps_append_structure(caps, structure);
            return caps;
        }

        return nullptr;
    }

    GstCaps *gstCapsNewEmptySimple(const char *media_type) const override
    {
        return gst_caps_new_empty_simple(media_type);
    }

    GstCaps *gstCapsNewEmpty() const override { return gst_caps_new_empty(); }

    GstProtectionMeta *gstBufferAddProtectionMeta(GstBuffer *buffer, GstStructure *info) const override
    {
        return gst_buffer_add_protection_meta(buffer, info);
    }

    GstProtectionMeta *gstBufferGetProtectionMeta(GstBuffer *buffer) const override
    {
        return gst_buffer_get_protection_meta(buffer);
    }

    GstMeta *gstBufferAddMeta(GstBuffer *buffer, const GstMetaInfo *info, gpointer params) const override
    {
        return gst_buffer_add_meta(buffer, info, params);
    }

    GstMeta *gstBufferGetMeta(GstBuffer *buffer, GType api) const override { return gst_buffer_get_meta(buffer, api); }

    gboolean gstBufferRemoveMeta(GstBuffer *buffer, GstMeta *meta) const override
    {
        return gst_buffer_remove_meta(buffer, meta);
    }

    gboolean gstStructureGetUint(const GstStructure *structure, const gchar *fieldname, guint *value) const override
    {
        return gst_structure_get_uint(structure, fieldname, value);
    }

    const GValue *gstStructureGetValue(const GstStructure *structure, const gchar *fieldname) const override
    {
        return gst_structure_get_value(structure, fieldname);
    }

    GstBuffer *gstValueGetBuffer(const GValue *value) const override { return gst_value_get_buffer(value); }

    GstEvent *gstEventNewStep(GstFormat format, guint64 amount, gdouble rate, gboolean flush,
                              gboolean intermediate) const override
    {
        return gst_event_new_step(format, amount, rate, flush, intermediate);
    }

    gdouble gstStreamVolumeGetVolume(GstStreamVolume *volume, GstStreamVolumeFormat format) const override
    {
        return gst_stream_volume_get_volume(volume, format);
    }

    void gstStreamVolumeSetVolume(GstStreamVolume *volume, GstStreamVolumeFormat format, gdouble val) const
    {
        gst_stream_volume_set_volume(volume, format, val);
    }

    gboolean gstStreamVolumeGetMute(GstStreamVolume *volume) const override
    {
        return gst_stream_volume_get_mute(volume);
    }

    void gstStreamVolumeSetMute(GstStreamVolume *volume, gboolean mute) const
    {
        return gst_stream_volume_set_mute(volume, mute);
    }

    GstElement *gstPipelineNew(const gchar *name) const override { return gst_pipeline_new(name); }

    GstPluginFeature *gstRegistryLookupFeature(GstRegistry *registry, const char *name) const override
    {
        return gst_registry_lookup_feature(registry, name);
    }

    guint64 gstAppSrcGetCurrentLevelBytes(GstAppSrc *appsrc) const override
    {
        return gst_app_src_get_current_level_bytes(appsrc);
    }

    GstEvent *gstEventNewFlushStart() const override { return gst_event_new_flush_start(); }

    GstEvent *gstEventNewFlushStop(gboolean reset_time) const override { return gst_event_new_flush_stop(reset_time); }

    guint64 gstAudioChannelGetFallbackMask(gint channels) const override
    {
        return gst_audio_channel_get_fallback_mask(channels);
    }

    void gstStructureSet(GstStructure *structure, const gchar *firstname, ...) const override;

    void gstMessageParseError(GstMessage *message, GError **gerror, gchar **debug) const override
    {
        return gst_message_parse_error(message, gerror, debug);
    }

    GstIterator *gstBinIterateSinks(GstBin *bin) const override { return gst_bin_iterate_sinks(bin); }

    GstIteratorResult gstIteratorNext(GstIterator *it, GValue *elem) const override
    {
        return gst_iterator_next(it, elem);
    }

    void gstIteratorFree(GstIterator *it) const override { return gst_iterator_free(it); }

    gboolean gstElementPostMessage(GstElement *element, GstMessage *message) const override
    {
        return gst_element_post_message(element, message);
    }

    GstMessage *gstMessageNewWarning(GstObject *src, GError *error, const gchar *debug) const override
    {
        return gst_message_new_warning(src, error, debug);
    }

    void gstMessageParseWarning(GstMessage *message, GError **gerror, gchar **debug) const override
    {
        gst_message_parse_warning(message, gerror, debug);
    }

    GstCaps *gstPadGetCurrentCaps(GstPad *pad) const override { return gst_pad_get_current_caps(pad); }

    GstStructure *gstCapsGetStructure(const GstCaps *caps, guint index) const override
    {
        return gst_caps_get_structure(caps, index);
    }

    const gchar *gstStructureGetName(const GstStructure *structure) const override
    {
        return gst_structure_get_name(structure);
    }

    bool gstStructureHasName(const GstStructure *structure, const gchar *name) const override
    {
        return gst_structure_has_name(structure, name);
    }

    gboolean gstObjectSetName(GstObject *object, const gchar *name) const override
    {
        return gst_object_set_name(object, name);
    }

    bool gstStructureHasField(const GstStructure *structure, const gchar *fieldname) const override
    {
        return gst_structure_has_field(structure, fieldname);
    }

    gboolean gstSegmentDoSeek(GstSegment *segment, gdouble rate, GstFormat format, GstSeekFlags flags,
                              GstSeekType startType, guint64 start, GstSeekType stopType, guint64 stop,
                              gboolean *update) const override
    {
        return gst_segment_do_seek(segment, rate, format, flags, startType, start, stopType, stop, update);
    }

    GstSample *gstSampleNew(GstBuffer *buffer, GstCaps *caps, const GstSegment *segment, GstStructure *info) const override
    {
        return gst_sample_new(buffer, caps, segment, info);
    }

    void gstSampleUnref(GstSample *sample) const override { gst_sample_unref(sample); }

    GstFlowReturn gstAppSrcPushSample(GstAppSrc *appsrc, GstSample *sample) const override
    {
        return gst_app_src_push_sample(appsrc, sample);
    }

    GstContext *gstContextNew(const gchar *context_type, gboolean persistent) const override
    {
        return gst_context_new(context_type, persistent);
    }

    GstStructure *gstContextWritableStructure(GstContext *context) const override
    {
        return gst_context_writable_structure(context);
    }

    void gstElementSetContext(GstElement *element, GstContext *context) const override
    {
        gst_element_set_context(element, context);
    }

    void gstContextUnref(GstContext *context) const override { gst_context_unref(context); }

    virtual GstAudioClippingMeta *gstBufferAddAudioClippingMeta(GstBuffer *buffer, GstFormat format, guint64 start,
                                                                guint64 end) const
    {
        return gst_buffer_add_audio_clipping_meta(buffer, format, start, end);
    }
};

}; // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_GST_WRAPPER_H_
