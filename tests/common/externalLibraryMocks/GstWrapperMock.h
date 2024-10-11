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

#ifndef FIREBOLT_RIALTO_WRAPPERS_GST_WRAPPER_MOCK_H_
#define FIREBOLT_RIALTO_WRAPPERS_GST_WRAPPER_MOCK_H_

#include "IGstWrapper.h"
#include <gmock/gmock.h>
#include <string>

namespace firebolt::rialto::wrappers
{
class GstWrapperMock : public IGstWrapper
{
public:
    GstWrapperMock() = default;
    virtual ~GstWrapperMock() = default;

    MOCK_METHOD(void, gstInit, (int *argc, char ***argv), (override));
    MOCK_METHOD(GstPlugin *, gstRegistryFindPlugin, (GstRegistry * registry, const gchar *name), (override));
    MOCK_METHOD(void, gstRegistryRemovePlugin, (GstRegistry * registry, GstPlugin *plugin), (override));
    MOCK_METHOD(void, gstObjectUnref, (gpointer object), (override));
    MOCK_METHOD(GstRegistry *, gstRegistryGet, (), (override));
    MOCK_METHOD(GstElementFactory *, gstElementFactoryFind, (const gchar *name), (override));
    MOCK_METHOD(gboolean, gstElementRegister, (GstPlugin * plugin, const gchar *name, guint rank, GType type),
                (override));
    MOCK_METHOD(GstElement *, gstElementFactoryMake, (const gchar *factoryname, const gchar *name), (override));
    MOCK_METHOD(gpointer, gstObjectRef, (gpointer object), (override));
    MOCK_METHOD(GstElement *, gstBinGetByName, (GstBin * bin, const gchar *name), (override));
    MOCK_METHOD(GstBus *, gstPipelineGetBus, (GstPipeline * pipeline), (override));
    MOCK_METHOD(void, gstMessageParseStateChanged,
                (GstMessage * message, GstState *oldstate, GstState *newstate, GstState *pending), (override));
    MOCK_METHOD(const gchar *, gstElementStateGetName, (GstState state), (override));
    MOCK_METHOD(GstStateChangeReturn, gstElementSetState, (GstElement * element, GstState state), (override));
    MOCK_METHOD(GstState, gstElementGetState, (GstElement * element), (override));
    MOCK_METHOD(GstState, gstElementGetPendingState, (GstElement * element), (override));
    MOCK_METHOD(GstObject *, gstElementGetParent, (const GstElement *elem), (const, override));
    MOCK_METHOD(gchar *, gstElementGetName, (GstElement * element), (const, override));
    MOCK_METHOD(gboolean, gstElementSendEvent, (GstElement * element, GstEvent *event), (const, override));
    MOCK_METHOD(void, gstAppSrcSetCallbacks,
                (GstAppSrc * appsrc, GstAppSrcCallbacks *callbacks, gpointer userData, GDestroyNotify notify),
                (override));
    MOCK_METHOD(void, gstAppSrcSetMaxBytes, (GstAppSrc * appsrc, guint64 max), (override));
    MOCK_METHOD(void, gstAppSrcSetStreamType, (GstAppSrc * appsrc, GstAppStreamType type), (override));
    MOCK_METHOD(gboolean, gstBinAdd, (GstBin * bin, GstElement *element), (override));
    MOCK_METHOD(void, gstBaseTransformSetInPlace, (GstBaseTransform * trans, gboolean in_place), (override));
    MOCK_METHOD(gboolean, gstElementSyncStateWithParent, (GstElement * element), (override));
    MOCK_METHOD(gboolean, gstElementLink, (GstElement * src, GstElement *dest), (override));
    MOCK_METHOD(GstPad *, gstElementGetStaticPad, (GstElement * element, const gchar *name), (override));
    MOCK_METHOD(GstPad *, gstGhostPadNew, (const gchar *name, GstPad *target), (override));
    MOCK_METHOD(void, gstPadSetQueryFunction, (GstPad * pad, GstPadQueryFunction query), (override));
    MOCK_METHOD(gboolean, gstPadSetActive, (GstPad * pad, gboolean active), (override));
    MOCK_METHOD(gboolean, gstPadSendEvent, (GstPad * pad, GstEvent *event), (override));
    MOCK_METHOD(gboolean, gstElementAddPad, (GstElement * element, GstPad *pad), (override));
    MOCK_METHOD(gboolean, gstElementSeek,
                (GstElement *, gdouble, GstFormat, GstSeekFlags, GstSeekType, gint64, GstSeekType, gint64), (override));
    MOCK_METHOD(void, gstElementNoMorePads, (GstElement * element), (override));
    MOCK_METHOD(GstElement *, gstElementFactoryCreate, (GstElementFactory * factory, const gchar *name), (override));
    MOCK_METHOD(GstCaps *, gstCapsFromString, (const gchar *string), (override));
    MOCK_METHOD(void, gstAppSrcSetCaps, (GstAppSrc * appsrc, const GstCaps *caps), (override));
    MOCK_METHOD(GstCaps *, gstAppSrcGetCaps, (GstAppSrc * appsrc), (override));
    MOCK_METHOD(gboolean, gstCapsIsEqual, (const GstCaps *caps1, const GstCaps *caps2), (override));
    MOCK_METHOD(gchar *, gstCapsToString, (const GstCaps *caps), (override));
    MOCK_METHOD(void, gstCapsUnref, (GstCaps * caps), (override));
    MOCK_METHOD(void, gstBusSetSyncHandler, (GstBus *, GstBusSyncHandler, gpointer, GDestroyNotify), (override));
    MOCK_METHOD(GstFlowReturn, gstAppSrcEndOfStream, (GstAppSrc *), (override));
    MOCK_METHOD(gboolean, gstElementQueryPosition, (GstElement *, GstFormat, gint64 *), (override));
    MOCK_METHOD(GstFlowReturn, gstAppSrcPushBuffer, (GstAppSrc *, GstBuffer *), (override));
    MOCK_METHOD(GstBuffer *, gstBufferNew, (), (override));
    MOCK_METHOD(GstBuffer *, gstBufferNewAllocate, (GstAllocator *, gsize, GstAllocationParams *), (override));
    MOCK_METHOD(gsize, gstBufferFill, (GstBuffer *, gsize, gconstpointer, gsize), (override));
    MOCK_METHOD(void, gstBufferUnref, (GstBuffer *), (override));
    MOCK_METHOD(gboolean, gstBufferMap, (GstBuffer * buffer, GstMapInfo *info, GstMapFlags flags), (override));
    MOCK_METHOD(void, gstBufferUnmap, (GstBuffer * buffer, GstMapInfo *info), (override));
    MOCK_METHOD(void, gstMessageUnref, (GstMessage *), (override));
    MOCK_METHOD(GstMessage *, gstBusTimedPopFiltered, (GstBus * bus, GstClockTime timeout, GstMessageType types),
                (override));
    MOCK_METHOD(void, gstDebugBinToDotFileWithTs, (GstBin * bin, GstDebugGraphDetails details, const gchar *file_name),
                (override));
    MOCK_METHOD(GstElementFactory *, gstElementGetFactory, (GstElement * element), (const, override));
    MOCK_METHOD(gboolean, gstElementFactoryListIsType, (GstElementFactory * factory, GstElementFactoryListType type),
                (const, override));
    MOCK_METHOD(GstCaps *, gstCapsCopy, (const GstCaps *caps), (const, override));
    MOCK_METHOD(void, gstCapsSetSimpleIntStub, (GstCaps * caps, const gchar *field, GType type, int value), (const));
    MOCK_METHOD(void, gstCapsSetSimpleStringStub, (GstCaps * caps, const gchar *field, GType type, const char *value),
                (const));
    MOCK_METHOD(void, gstCapsSetSimpleBufferStub, (GstCaps * caps, const gchar *field, GType type, GstBuffer *value),
                (const));
    MOCK_METHOD(void, gstCapsSetSimpleBooleanStub,
                (GstCaps * caps, const gchar *field, GType type, const gboolean value), (const));
    MOCK_METHOD(void, gstCapsSetSimpleUintStub, (GstCaps * caps, const gchar *field, GType type, const unsigned value),
                (const));
    MOCK_METHOD(void, gstCapsSetSimpleBitMaskStub,
                (GstCaps * caps, const gchar *field, GType type, const uint64_t value), (const));
    MOCK_METHOD(void, gstCapsSetSimpleFractionStub,
                (GstCaps * caps, const gchar *field, GType type, int value1, int value2), (const));
    MOCK_METHOD(GstCaps *, gstCapsNewSimpleIntStub,
                (const char *media_type, const char *fieldname, GType type, int value), (const));
    MOCK_METHOD(void, gstMessageParseQos,
                (GstMessage * message, gboolean *live, guint64 *running_time, guint64 *stream_time, guint64 *timestamp,
                 guint64 *duration),
                (const));
    MOCK_METHOD(void, gstMessageParseQosStats,
                (GstMessage * message, GstFormat *format, guint64 *processed, guint64 *dropped), (const));
    MOCK_METHOD(const gchar *, gstElementClassGetMetadata, (GstElementClass * klass, const gchar *key), (const));
    MOCK_METHOD(const gchar *, gstFormatGetName, (GstFormat format), (const));
    MOCK_METHOD(GstSegment *, gstSegmentNew, (), (const, override));
    MOCK_METHOD(void, gstSegmentInit, (GstSegment * segment, GstFormat format), (const, override));
    MOCK_METHOD(void, gstSegmentFree, (GstSegment * segment), (const, override));
    MOCK_METHOD(GstEvent *, gstEventNewSegment, (const GstSegment *segment), (const, override));
    MOCK_METHOD(GstEvent *, gstEventNewCustom, (GstEventType type, GstStructure *structure), (const, override));
    MOCK_METHOD(GstStructure *, gstStructureNewDoubleStub,
                (const gchar *name, const gchar *firstfield, GType type, double value), (const));
    MOCK_METHOD(GstStructure *, gstStructureNewBoolStub,
                (const gchar *name, const gchar *firstfield, GType type, gboolean value), (const));
    MOCK_METHOD(GstStructure *, gstStructureNewBufferStub,
                (const gchar *name, const gchar *firstfield, GType type, GstBuffer *value), (const));
    MOCK_METHOD(GstStructure *, gstStructureNewUintStub,
                (const gchar *name, const gchar *firstfield, GType type, uint32_t value), (const));
    MOCK_METHOD(GstStructure *, gstStructureNewStringStub,
                (const gchar *name, const gchar *firstfield, GType type, const char *value), (const));
    MOCK_METHOD(void, gstByteWriterInitWithData,
                (GstByteWriter * writer, guint8 *data, guint size, gboolean initialized), (const, override));
    MOCK_METHOD(gboolean, gstByteWriterPutUint16Be, (GstByteWriter * writer, guint16 val), (const, override));
    MOCK_METHOD(gboolean, gstByteWriterPutUint32Be, (GstByteWriter * writer, guint32 val), (const, override));
    MOCK_METHOD(GstBuffer *, gstBufferNewWrapped, (gpointer data, gsize size), (const, override));
    MOCK_METHOD(GstCaps *, gstCodecUtilsOpusCreateCapsFromHeader, (gconstpointer data, guint size), (const, override));
    MOCK_METHOD(gboolean, gstCapsIsStrictlyEqual, (const GstCaps *caps1, const GstCaps *caps2), (const));
    MOCK_METHOD(gboolean, gstCapsCanIntersect, (const GstCaps *caps1, const GstCaps *caps2), (const));
    MOCK_METHOD(GstCaps *, gstStaticCapsGet, (GstStaticCaps * staticCaps), (const));
    MOCK_METHOD(GList *, gstElementFactoryListGetElements, (GstElementFactoryListType type, GstRank minrank), (const));
    MOCK_METHOD(const GList *, gstElementFactoryGetStaticPadTemplates, (GstElementFactory * factory), (const));
    MOCK_METHOD(void, gstPluginFeatureListFree, (GList * list), (const));
    MOCK_METHOD(GstCaps *, gstCapsNewEmptySimple, (const char *media_type), (const));
    MOCK_METHOD(GstCaps *, gstCapsNewEmpty, (), (const));
    MOCK_METHOD(GstProtectionMeta *, gstBufferAddProtectionMeta, (GstBuffer * buffer, GstStructure *info), (const));
    MOCK_METHOD(GstMeta *, gstBufferAddMeta, (GstBuffer * buffer, const GstMetaInfo *info, gpointer params),
                (const, override));
    MOCK_METHOD(GstMeta *, gstBufferGetMeta, (GstBuffer * buffer, GType api), (const, override));
    MOCK_METHOD(gboolean, gstBufferRemoveMeta, (GstBuffer * buffer, GstMeta *meta), (const, override));
    MOCK_METHOD(gboolean, gstStructureGetUint64,
                (const GstStructure *structure, const gchar *fieldname, guint64 *value), (const));
    MOCK_METHOD(void, gstStructureFree, (GstStructure * structure), (const));
    MOCK_METHOD(GstEvent *, gstEventNewStep,
                (GstFormat format, guint64 amount, gdouble rate, gboolean flush, gboolean intermediate), (const));
    MOCK_METHOD(gdouble, gstStreamVolumeGetVolume, (GstStreamVolume * volume, GstStreamVolumeFormat format),
                (const, override));
    MOCK_METHOD(void, gstStreamVolumeSetVolume, (GstStreamVolume * volume, GstStreamVolumeFormat format, gdouble val),
                (const, override));
    MOCK_METHOD(gboolean, gstStreamVolumeGetMute, (GstStreamVolume * volume), (const, override));
    MOCK_METHOD(void, gstStreamVolumeSetMute, (GstStreamVolume * volume, gboolean mute), (const, override));
    MOCK_METHOD(GstElement *, gstPipelineNew, (const gchar *name), (const, override));
    MOCK_METHOD(GstPluginFeature *, gstRegistryLookupFeature, (GstRegistry * registry, const char *name),
                (const, override));
    MOCK_METHOD(guint64, gstAppSrcGetCurrentLevelBytes, (GstAppSrc * appsrc), (const, override));
    MOCK_METHOD(GstEvent *, gstEventNewFlushStart, (), (const, override));
    MOCK_METHOD(GstEvent *, gstEventNewFlushStop, (gboolean reset_time), (const, override));
    MOCK_METHOD(GstObject *, gstObjectParent, (gpointer object), (const, override));
    MOCK_METHOD(GstObject *, gstObjectCast, (gpointer object), (const, override));
    MOCK_METHOD(guint64, gstAudioChannelGetFallbackMask, (gint channels), (const, override));
    MOCK_METHOD(void, gstStructureSetUintStub,
                (GstStructure * structure, const gchar *firstname, GType type, uint32_t value), (const));
    MOCK_METHOD(void, gstStructureSetStringStub,
                (GstStructure * structure, const gchar *firstname, GType type, const char *value), (const));
    MOCK_METHOD(void, gstMessageParseError, (GstMessage * message, GError **gerror, gchar **debug), (const));
    MOCK_METHOD(GstIterator *, gstBinIterateSinks, (GstBin * bin), (const, override));
    MOCK_METHOD(GstIterator *, gstBinIterateRecurse, (GstBin * bin), (const, override));
    MOCK_METHOD(GstIteratorResult, gstIteratorNext, (GstIterator * it, GValue *elem), (const, override));
    MOCK_METHOD(void, gstIteratorResync, (GstIterator * it), (const, override));
    MOCK_METHOD(void, gstIteratorFree, (GstIterator * it), (const, override));
    MOCK_METHOD(gboolean, gstElementPostMessage, (GstElement * element, GstMessage *message), (const, override));
    MOCK_METHOD(GstMessage *, gstMessageNewWarning, (GstObject * src, GError *error, const gchar *debug),
                (const, override));
    MOCK_METHOD(void, gstMessageParseWarning, (GstMessage * message, GError **gerror, gchar **debug), (const, override));
    MOCK_METHOD(GstStructure *, gstCapsGetStructure, (const GstCaps *caps, guint index), (const, override));
    MOCK_METHOD(gboolean, gstObjectSetName, (GstObject * object, const gchar *name), (const, override));
    MOCK_METHOD(gboolean, gstSegmentDoSeek,
                (GstSegment *, gdouble, GstFormat, GstSeekFlags, GstSeekType, guint64, GstSeekType, guint64, gboolean *),
                (const, override));
    MOCK_METHOD(GstContext *, gstContextNew, (const gchar *context_type, gboolean persistent), (const, override));
    MOCK_METHOD(GstStructure *, gstContextWritableStructure, (GstContext * context), (const, override));
    MOCK_METHOD(void, gstElementSetContext, (GstElement * element, GstContext *context), (const, override));
    MOCK_METHOD(void, gstContextUnref, (GstContext * context), (const, override));
    MOCK_METHOD(GstSample *, gstSampleNew,
                (GstBuffer * buffer, GstCaps *caps, const GstSegment *segment, GstStructure *info), (const, override));
    MOCK_METHOD(void, gstSampleUnref, (GstSample * sample), (const, override));
    MOCK_METHOD(GstFlowReturn, gstAppSrcPushSample, (GstAppSrc * appsrc, GstSample *sample), (const, override));
    MOCK_METHOD(bool, gstStructureHasName, (const GstStructure *structure, const gchar *name), (const, override));
    MOCK_METHOD(bool, gstStructureHasField, (const GstStructure *structure, const gchar *fieldname), (const, override));
    MOCK_METHOD(GstAudioClippingMeta *, gstBufferAddAudioClippingMeta,
                (GstBuffer * buffer, GstFormat format, guint64 start, guint64 end), (const, override));
    MOCK_METHOD(GstPad *, gstElementGetStaticPad, (GstElement * element, const gchar *name), (const, override));
    MOCK_METHOD(GstPad *, gstBaseSinkPad, (GstElement * element), (const, override));

    GstCaps *gstCapsNewSimple(const char *media_type, const char *fieldname, ...) const override
    {
        va_list args;
        const gchar *kProperty = fieldname;

        va_start(args, fieldname);

        GType intType = va_arg(args, GType);
        int intValue = va_arg(args, int);
        GstCaps *result = gstCapsNewSimpleIntStub(media_type, kProperty, intType, intValue);

        va_end(args);

        return result;
    }

    void gstCapsSetSimple(GstCaps *caps, const gchar *field, ...) const override
    {
        va_list args;
        const gchar *kProperty = field;

        va_start(args, field);

        while (NULL != kProperty)
        {
            GType type = va_arg(args, GType);
            if (g_type_is_a(type, G_TYPE_INT))
            {
                int intValue = va_arg(args, int);
                gstCapsSetSimpleIntStub(caps, kProperty, type, intValue);
            }
            else if (g_type_is_a(type, G_TYPE_STRING))
            {
                const char *kVal = va_arg(args, const char *);
                gstCapsSetSimpleStringStub(caps, kProperty, type, kVal);
            }
            else if (g_type_is_a(type, GST_TYPE_BUFFER))
            {
                GstBuffer *buf = va_arg(args, GstBuffer *);
                gstCapsSetSimpleBufferStub(caps, kProperty, type, buf);
            }
            else if (g_type_is_a(type, G_TYPE_BOOLEAN))
            {
                gboolean val = va_arg(args, gboolean);
                gstCapsSetSimpleBooleanStub(caps, kProperty, type, val);
            }
            else if (g_type_is_a(type, G_TYPE_UINT))
            {
                unsigned val = va_arg(args, unsigned);
                gstCapsSetSimpleUintStub(caps, kProperty, type, val);
            }
            else if (g_type_is_a(type, GST_TYPE_BITMASK))
            {
                uint64_t val = va_arg(args, uint64_t);
                gstCapsSetSimpleBitMaskStub(caps, kProperty, type, val);
            }
            else if (g_type_is_a(type, GST_TYPE_FRACTION))
            {
                int val1 = va_arg(args, int);
                int val2 = va_arg(args, int);
                gstCapsSetSimpleFractionStub(caps, kProperty, type, val1, val2);
            }
            kProperty = va_arg(args, const gchar *);
        }

        va_end(args);
    }

    GstStructure *gstStructureNew(const gchar *name, const gchar *firstfield, ...) const override
    {
        GstStructure *structure{nullptr};
        va_list args;
        const gchar *kProperty = firstfield;

        va_start(args, firstfield);

        while (NULL != kProperty)
        {
            GType valueType = va_arg(args, GType);
            if (g_type_is_a(valueType, G_TYPE_DOUBLE))
            {
                double value = va_arg(args, double);
                structure = gstStructureNewDoubleStub(name, kProperty, valueType, value);
            }
            else if (g_type_is_a(valueType, G_TYPE_BOOLEAN))
            {
                gboolean value = va_arg(args, gboolean);
                structure = gstStructureNewBoolStub(name, kProperty, valueType, value);
            }
            else if (g_type_is_a(valueType, GST_TYPE_BUFFER))
            {
                GstBuffer *value = va_arg(args, GstBuffer *);
                structure = gstStructureNewBufferStub(name, kProperty, valueType, value);
            }
            else if (g_type_is_a(valueType, G_TYPE_UINT))
            {
                uint32_t value = va_arg(args, guint32);
                structure = gstStructureNewUintStub(name, kProperty, valueType, value);
            }
            else if (g_type_is_a(valueType, G_TYPE_STRING))
            {
                const char *kValue = va_arg(args, const char *);
                structure = gstStructureNewStringStub(name, kProperty, valueType, kValue);
            }
            kProperty = va_arg(args, const gchar *);
        }

        va_end(args);
        return structure;
    }

    void gstStructureSet(GstStructure *structure, const gchar *firstname, ...) const override
    {
        va_list args;
        const gchar *kField = firstname;

        va_start(args, firstname);

        while (NULL != kField)
        {
            GType type = va_arg(args, GType);
            if (g_type_is_a(type, G_TYPE_STRING))
            {
                const char *kVal = va_arg(args, const char *);
                gstStructureSetStringStub(structure, kField, type, kVal);
            }
            else if (g_type_is_a(type, G_TYPE_UINT))
            {
                unsigned kVal = va_arg(args, unsigned);
                gstStructureSetUintStub(structure, kField, type, kVal);
            }
            kField = va_arg(args, const gchar *);
        }

        va_end(args);
    }
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_GST_WRAPPER_MOCK_H_
