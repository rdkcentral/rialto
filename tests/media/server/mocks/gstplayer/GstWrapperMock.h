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

#ifndef FIREBOLT_RIALTO_SERVER_GST_WRAPPER_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_GST_WRAPPER_MOCK_H_

#include "IGstWrapper.h"
#include <gmock/gmock.h>
#include <string>

namespace firebolt::rialto::server
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
    MOCK_METHOD(guint, gstBusAddWatch, (GstBus * bus, GstBusFunc func, gpointer user_data), (override));
    MOCK_METHOD(void, gstMessageParseStateChanged,
                (GstMessage * message, GstState *oldstate, GstState *newstate, GstState *pending), (override));
    MOCK_METHOD(const gchar *, gstElementStateGetName, (GstState state), (override));
    MOCK_METHOD(GstStateChangeReturn, gstElementSetState, (GstElement * element, GstState state), (override));
    MOCK_METHOD(GstState, gstElementGetState, (GstElement * element), (override));
    MOCK_METHOD(GstState, gstElementGetPendingState, (GstElement * element), (override));
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
    MOCK_METHOD(GstStructure *, gstStructureNewStub,
                (const gchar *name, const gchar *firstfield, GType type, double value), (const));
    MOCK_METHOD(void, gstByteWriterInitWithData,
                (GstByteWriter * writer, guint8 *data, guint size, gboolean initialized), (const, override));
    MOCK_METHOD(gboolean, gstByteWriterPutUint16Be, (GstByteWriter * writer, guint16 val), (const, override));
    MOCK_METHOD(gboolean, gstByteWriterPutUint32Be, (GstByteWriter * writer, guint32 val), (const, override));
    MOCK_METHOD(GstBuffer *, gstBufferNewWrapped, (gpointer data, gsize size), (const, override));
    MOCK_METHOD(GstCaps *, gstCodecUtilsOpusCreateCapsFromHeader, (gconstpointer data, guint size), (const, override));
    MOCK_METHOD(gboolean, gstCapsIsSubset, (const GstCaps *subset, const GstCaps *superset), (const));
    MOCK_METHOD(gboolean, gstCapsIsStrictlyEqual, (const GstCaps *caps1, const GstCaps *caps2), (const));
    MOCK_METHOD(gboolean, gstCapsCanIntersect, (const GstCaps *caps1, const GstCaps *caps2), (const));
    MOCK_METHOD(GstCaps *, gstStaticCapsGet, (GstStaticCaps * staticCaps), (const));
    MOCK_METHOD(GList *, gstElementFactoryListGetElements, (GstElementFactoryListType type, GstRank minrank), (const));
    MOCK_METHOD(const GList *, gstElementFactoryGetStaticPadTemplates, (GstElementFactory * factory), (const));
    MOCK_METHOD(void, gstPluginFeatureListFree, (GList * list), (const));

    void gstCapsSetSimple(GstCaps *caps, const gchar *field, ...) const override
    {
        va_list args;
        const gchar *property = field;

        va_start(args, field);

        while (NULL != property)
        {
            GType intType = va_arg(args, GType);
            int intValue = va_arg(args, int);
            gstCapsSetSimpleIntStub(caps, property, intType, intValue);
            property = va_arg(args, const gchar *);
        }

        va_end(args);
    }

    GstStructure *gstStructureNew(const gchar *name, const gchar *firstfield, ...) const
    {
        GstStructure *structure{nullptr};
        va_list args;
        const gchar *property = firstfield;

        va_start(args, firstfield);

        while (NULL != property)
        {
            GType valueType = va_arg(args, GType);
            double value = va_arg(args, double);
            structure = gstStructureNewStub(name, property, valueType, value);
            property = va_arg(args, const gchar *);
        }

        va_end(args);
        return structure;
    }
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_WRAPPER_MOCK_H_
