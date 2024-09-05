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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_GST_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_GST_WRAPPER_H_

#include <gst/app/gstappsrc.h>
#include <gst/audio/audio.h>
#include <gst/audio/streamvolume.h>
#include <gst/base/gstbasetransform.h>
#include <gst/base/gstbytewriter.h>
#include <gst/gst.h>
#include <memory>
#include <stdint.h>
#include <string>

namespace firebolt::rialto::wrappers
{
class IGstWrapper;

/**
 * @brief IGstWrapper factory class, for the IGstWrapper singleton object.
 */
class IGstWrapperFactory
{
public:
    IGstWrapperFactory() = default;
    virtual ~IGstWrapperFactory() = default;

    /**
     * @brief Gets the IGstWrapperFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IGstWrapperFactory> getFactory();

    /**
     * @brief Gets a IGstWrapper singleton object.
     *
     * @retval the wrapper instance or null on error.
     */
    virtual std::shared_ptr<IGstWrapper> getGstWrapper() = 0;
};

class IGstWrapper
{
public:
    IGstWrapper() = default;
    virtual ~IGstWrapper() = default;

    IGstWrapper(const IGstWrapper &) = delete;
    IGstWrapper &operator=(const IGstWrapper &) = delete;
    IGstWrapper(IGstWrapper &&) = delete;
    IGstWrapper &operator=(IGstWrapper &&) = delete;

    /**
     * @brief Initalise gstreamer.
     *
     * @param[in] argc    : The count of command line arguments.
     * @param[in] argv    : Vector of C strings each containing a command line argument.
     */
    virtual void gstInit(int *argc, char ***argv) = 0;

    /**
     * @brief Finds the plugin with the given name.
     *
     * @param[in] registry  : The registry to search.
     * @param[in] name      : The plugin name to find.
     *
     * @retval The GstPlugin or NULL if not found.
     */
    virtual GstPlugin *gstRegistryFindPlugin(GstRegistry *registry, const gchar *name) = 0;

    /**
     * @brief Removes the plugin with the given name from the registry.
     *
     * @param[in] registry  : The registry to remove the plugin from.
     * @param[in] plugin     : The plugin to remove.
     */
    virtual void gstRegistryRemovePlugin(GstRegistry *registry, GstPlugin *plugin) = 0;

    /**
     * @brief Retrieves the plugin registry.
     *
     * @retval The plugin registry or NULL on error.
     */
    virtual GstRegistry *gstRegistryGet() = 0;

    /**
     * @brief Decrements the object reference counter.
     *
     * @param[in] object  : GstObject to unreference.
     */
    virtual void gstObjectUnref(gpointer object) = 0;

    /**
     * @brief Find an element factory.
     *
     * @param[in] name  : Name of element to find.
     *
     * @retval GstElementFactory object or NULL on error.
     */
    virtual GstElementFactory *gstElementFactoryFind(const gchar *name) = 0;

    /**
     * @brief Create a new element factory.
     *
     * @param[in] plugin    : GstPlugin to register or NULL if static.
     * @param[in] name      : Name of the element.
     * @param[in] rank      : Rank of the element.
     * @param[in] type      : GType of the element.
     *
     * @retval TRUE on success, false otherwise.
     */
    virtual gboolean gstElementRegister(GstPlugin *plugin, const gchar *name, guint rank, GType type) = 0;

    /**
     * @brief Create a new element of type returned by the requested factory.
     *
     * @param[in] factoryname   : Factory to use to create the element.
     * @param[in] name          : Name to call the element, if NULL a unique name is created.
     *
     * @retval GstElement object or NULL on error.
     */
    virtual GstElement *gstElementFactoryMake(const gchar *factoryname, const gchar *name) = 0;

    /**
     * @brief Get the element type returned by the requested factory.
     *
     * @param[in] factory   : Factory
     *
     * @retval GType that the factory produces
     */
    virtual GType gstElementFactoryGetElementType(GstElementFactory *factory) = 0;

    /**
     * @brief Increment reference count on object.
     *
     * @param[in] object   : Object to increment.
     *
     * @retval A pointer to the object.
     */
    virtual gpointer gstObjectRef(gpointer object) = 0;

    /**
     * @brief Get the parent of this object. This is not thread-safe by default (i.e. you will have to make sure the
     *        object lock is taken yourself). If in doubt use gst_object_get_parent instead.
     *
     * @param[in] object   : a pointer to the object
     *
     * @retval parent of object, this can be NULL if object has no parent.
     */
    virtual GstObject *gstObjectParent(gpointer object) const = 0;

    /**
     * @brief Cast to GstObject pointer
     *
     * @param[in] object   : a GstObject
     *
     * @retval GstObject pointer
     */
    virtual GstObject *gstObjectCast(gpointer object) const = 0;

    /**
     * @brief Get the element from the bin.
     *
     * @param[in] bin    : The bin to search.
     * @param[in] name   : The name of the element.
     *
     * @retval The element or NULL on error.
     */
    virtual GstElement *gstBinGetByName(GstBin *bin, const gchar *name) = 0;

    /**
     * @brief Gets the bus of the pipeline.
     *
     * @param[in] pipeline: The pipeline to use.
     */
    virtual GstBus *gstPipelineGetBus(GstPipeline *pipeline) = 0;

    /**
     * @brief Sets the synchronous handler on the bus.
     *
     * @param[in] bus       : The bus for the watch.
     * @param[in] func      : The message callback function.
     * @param[in] user_data : Data to be passed to the function.
     * @param[in] notify    : called when user_data becomes unused
     */
    virtual void gstBusSetSyncHandler(GstBus *bus, GstBusSyncHandler func, gpointer user_data, GDestroyNotify notify) = 0;

    /**
     * @brief Gets the old and new states from the message.
     *
     * @param[in] message   : Messge to extract the states from.
     * @param[out] oldstate : The old states.
     * @param[out] newstate : The new states.
     * @param[out] pending  : The pending states.
     */
    virtual void gstMessageParseStateChanged(GstMessage *message, GstState *oldstate, GstState *newstate,
                                             GstState *pending) = 0;

    /**
     * @brief Gets the state as a string.
     *
     * @param[in] state: The state.
     *
     * @retval The string of the state.
     */
    virtual const gchar *gstElementStateGetName(GstState state) = 0;

    /**
     * @brief Sets the state of the element.
     *
     * @param[in] element : A GstElement to change state of.
     * @param[in] state   : The element's new GstState.
     *
     * @retval Result of the state change using GstStateChangeReturn. MT safe.
     */
    virtual GstStateChangeReturn gstElementSetState(GstElement *element, GstState state) = 0;

    /**
     * @brief Gets the state of the element.
     *
     * @param[in] element : A GstElement to get state of.
     *
     * @retval The element's current GstState.
     */
    virtual GstState gstElementGetState(GstElement *element) = 0;

    /**
     * @brief Returns a copy of the name of elem. Caller should g_free the return value after usage. For a nameless
     *        element, this returns NULL, which you can safely g_free as well.
     *
     * @param[in] element : a GstElement to get the name of elem.
     *
     * @retval the name of elem. g_free after usage. MT safe.
     */
    virtual gchar *gstElementGetName(GstElement *element) const = 0;

    /**
     * @brief Gets the pending state of the element.
     *
     * @param[in] element : A GstElement to get pending state of.
     *
     * @retval The element's pending GstState.
     */
    virtual GstState gstElementGetPendingState(GstElement *element) = 0;

    /**
     * @brief Get the parent of an element.
     *
     * @param[in] element : a GstElement to get the parent of.
     *
     * @retval The parent of an element.
     */
    virtual GstObject *gstElementGetParent(const GstElement *elem) const = 0;

    /**
     * @brief Sends an event to an element.
     *
     * @param[in] element : a GstElement to send the event to.
     * @param[in] event   : the GstEvent to send to the element.
     *
     * @retval TRUE if the event was handled.
     */
    virtual gboolean gstElementSendEvent(GstElement *element, GstEvent *event) const = 0;

    /**
     * @brief Set callbacks to be installed on the appsrc.
     *
     * @param[in] appsrc    : The appsrc.
     * @param[in] callbacks : The callbacks to be installed.
     * @param[in] userData  : The user data to be passed to the callbacks.
     * @param[in] notify    : The destroy function to be called on the destruction of the appsrc.
     */
    virtual void gstAppSrcSetCallbacks(GstAppSrc *appsrc, GstAppSrcCallbacks *callbacks, gpointer userData,
                                       GDestroyNotify notify) = 0;

    /**
     * @brief Set the maximum bytes that can be set on the appsrc queue.
     *
     * @param[in] appsrc    : The appsrc.
     * @param[in] max       : Max bytes to set.
     */
    virtual void gstAppSrcSetMaxBytes(GstAppSrc *appsrc, guint64 max) = 0;

    /**
     * @brief Set the stream type on the appsrc.
     *
     * @param[in] appsrc    : The appsrc.
     * @param[in] type      : The new stream type.
     */
    virtual void gstAppSrcSetStreamType(GstAppSrc *appsrc, GstAppStreamType type) = 0;

    /**
     * @brief Indicates to the appsrc element that the last buffer queued in the element is the last buffer of the stream.
     *
     * @param[in] appsrc    : The appsrc.
     */
    virtual GstFlowReturn gstAppSrcEndOfStream(GstAppSrc *appsrc) = 0;

    /**
     * @brief Add an element to the bin.
     *
     * @param[in] bin       : The bin.
     * @param[in] element   : The element to add.
     *
     * @retval TRUE on success, FALSE otherwise.
     */
    virtual gboolean gstBinAdd(GstBin *bin, GstElement *element) = 0;

    /**
     * @brief Set whether the transform should be inplace.
     *
     * @param[in] trans     : The transform to modify.
     * @param[in] in_place  : Bool to determine if in_place buffers should be used.
     */
    virtual void gstBaseTransformSetInPlace(GstBaseTransform *trans, gboolean in_place) = 0;

    /**
     * @brief Change state of element to the same as parent element.
     *
     * @param[in] element   : The element to change.
     *
     * @retval TRUE on success, FALSE otherwise.
     */
    virtual gboolean gstElementSyncStateWithParent(GstElement *element) = 0;

    /**
     * @brief Link the src and dest.
     *
     * @param[in] src   : The src element.
     * @param[in] dest  : The dest element.
     *
     * @retval TRUE on success, FALSE otherwise.
     */
    virtual gboolean gstElementLink(GstElement *src, GstElement *dest) = 0;

    /**
     * @brief Gets the pad from the element.
     *
     * @param[in] element   : Element to search.
     * @param[in] name      : Name of pad.
     *
     * @retval The requested pad, NULL otherwise.
     */
    virtual GstPad *gstElementGetStaticPad(GstElement *element, const gchar *name) = 0;

    /**
     * @brief Queries an element (usually top-level pipeline or playbin element) for the stream position in nanoseconds.
     *
     * @param[in] element : a GstElement to invoke the position query on.
     * @param[in] format  : the GstFormat requested
     * @param[in] cur     : a location in which to store the current position, or NULL.
     *
     * @retval The requested pad, NULL otherwise.
     */
    virtual gboolean gstElementQueryPosition(GstElement *element, GstFormat format, gint64 *cur) = 0;

    /**
     * @brief Create a new ghost pad.
     *
     * @param[in] name      : The name of the new pad, NULL to set default.
     * @param[in] target    : The pad to ghost.
     *
     * @retval  The pad, NULL otherwise.
     */
    virtual GstPad *gstGhostPadNew(const gchar *name, GstPad *target) = 0;

    /**
     * @brief Set the query function on the pad.
     *
     * @param[in] pad   : The pad.
     * @param[in] query : Query to set.
     */
    virtual void gstPadSetQueryFunction(GstPad *pad, GstPadQueryFunction query) = 0;

    /**
     * @brief Activate/Deactivate the pad.
     *
     * @param[in] pad       : The pad to change.
     * @param[in] active    : Whether the pad should be active or not.
     *
     * @retval TRUE on success, FALSE otherwise.
     */
    virtual gboolean gstPadSetActive(GstPad *pad, gboolean active) = 0;

    /**
     * @brief Sends the event to the pad. This function can be used by applications to send events in the pipeline.
     *
     * @param[in] pad       : a GstPad to send the event to.
     * @param[in] event    : the GstEvent to send to the pad.
     *
     * @retval TRUE on success, FALSE otherwise.
     */
    virtual gboolean gstPadSendEvent(GstPad *pad, GstEvent *event) = 0;

    /**
     * @brief Adds a pad to the element.
     *
     * @param[in] element   : Element to add pad to.
     * @param[in] pad       : Pad to add.
     *
     * @retval TRUE on success, FALSE otherwise.
     */
    virtual gboolean gstElementAddPad(GstElement *element, GstPad *pad) = 0;

    /**
     * @brief Sends a seek event to an element. See gst_event_new_seek for the details of the parameters. The seek event
     * is sent to the element using gst_element_send_event.
     *
     * @param[in] element    : a GstElement to send the event to.
     * @param[in] rate       : The new playback rate
     * @param[in] format     : The format of the seek values
     * @param[in] flags      : The optional seek flags.
     * @param[in] start_type : The type and flags for the new start position
     * @param[in] start      : The value of the new start position
     * @param[in] stop_type  : The type and flags for the new stop position
     * @param[in] stop       : The value of the new stop position
     *
     * @retval GstElement on success, NULL otherwise.
     */
    virtual gboolean gstElementSeek(GstElement *element, gdouble rate, GstFormat format, GstSeekFlags flags,
                                    GstSeekType start_type, gint64 start, GstSeekType stop_type, gint64 stop) = 0;

    /**
     * @brief Signal no more pads to be added to the element.
     *
     * @param[in] element   : The element.
     */
    virtual void gstElementNoMorePads(GstElement *element) = 0;

    /**
     * @brief Create a new element.
     *
     * @param[in] factory   : The factory to use for the element.
     * @param[in] name      : The name of the element or NULL to create a unique name.
     *
     * @retval GstElement on success, NULL otherwise.
     */
    virtual GstElement *gstElementFactoryCreate(GstElementFactory *factory, const gchar *name) = 0;

    /**
     * @brief Converts a caps string to GstCaps.
     *
     * @param[in] string   : String of caps.
     *
     * @retval GstCaps.
     */
    virtual GstCaps *gstCapsFromString(const gchar *string) = 0;

    /**
     * @brief Sets the caps on the appsrc.
     *
     * @param[in] appsrc   : The app src.
     * @param[in] caps     : Caps to set.
     */
    virtual void gstAppSrcSetCaps(GstAppSrc *appsrc, const GstCaps *caps) = 0;

    /**
     * @brief Get the caps set on the appsrc.
     *
     * @param[in] appsrc   : The app src.
     *
     * @retval The caps.
     */
    virtual GstCaps *gstAppSrcGetCaps(GstAppSrc *appsrc) = 0;

    /**
     * @brief Adds a buffer to the queue of buffers that the appsrc element will push to its source pad.
     * This function takes ownership of the buffer.
     * When the block property is TRUE, this function can block until free space becomes available in the queue.
     *
     * @param[in] appsrc   : The app src.
     * @param[in] buffer   : a GstBuffer to push
     *
     * @retval GST_FLOW_OK when the buffer was successfuly queued. GST_FLOW_WRONG_STATE when appsrc is not PAUSED or
     *         PLAYING. GST_FLOW_UNEXPECTED when EOS occured.
     */
    virtual GstFlowReturn gstAppSrcPushBuffer(GstAppSrc *appsrc, GstBuffer *buffer) = 0;

    /**
     * @brief Compare two sets of caps.
     *
     * @param[in] caps1 : First set of caps.
     * @param[in] caps2 : Second set of caps.
     *
     * @retval TRUE if equal, FALSE otherwise.
     */
    virtual gboolean gstCapsIsEqual(const GstCaps *caps1, const GstCaps *caps2) = 0;

    /**
     * @brief Convert GstCaps to string.
     *
     * @param[in] caps  : GstCaps to convert.
     *
     * @retval String representation of caps.
     */
    virtual gchar *gstCapsToString(const GstCaps *caps) = 0;

    /**
     * @brief Unreference a GstCaps.
     *
     * @param[in] caps  : GstCaps to dereference.
     *
     * @retval String representation of caps.
     */
    virtual void gstCapsUnref(GstCaps *caps) = 0;

    /**
     * @brief Creates a newly allocated buffer without any data.
     *
     * @retval the new GstBuffer.
     */
    virtual GstBuffer *gstBufferNew() = 0;

    /**
     * @brief Tries to create a newly allocated buffer with data of the given size and extra parameters from allocator.
     *
     * If the requested amount of memory can't be allocated, NULL will be returned.
     * The allocated buffer memory is not cleared. When allocator is NULL, the default memory allocator will be used.
     * Note that when size == 0, the buffer will not have memory associated with it.
     *
     * @param[in] allocator : the GstAllocator to use, or NULL to use the default allocator
     * @param[in] size      : the size in bytes of the new buffer's data.
     * @param[in] params    : optional parameters
     *
     * @retval a new GstBuffer
     */
    virtual GstBuffer *gstBufferNewAllocate(GstAllocator *allocator, gsize size, GstAllocationParams *params) = 0;

    /**
     * @brief Copies size bytes from src to buffer at offset.
     *
     * @param[in] buffer : a GstBuffer.
     * @param[in] offset : the offset to fill
     * @param[in] src    : the source address
     * @param[in] size   : the size to fill
     *
     * @retval The amount of bytes copied. This value can be lower than size when buffer did not contain enough data.
     */
    virtual gsize gstBufferFill(GstBuffer *buffer, gsize offset, gconstpointer src, gsize size) = 0;

    /**
     * @brief Decreases the refcount of the buffer. If the refcount reaches 0, the buffer with the associated
     *        metadata and memory will be freed.
     *
     * @param[in] buf : a GstBuffer.
     */
    virtual void gstBufferUnref(GstBuffer *buf) = 0;

    /**
     * @brief Fills info with the GstMapInfo of all merged memory blocks in buffer.
     *
     * @param[in] buffer : a GstBuffer.
     * @param[out] info  : info about the mapping
     * @param[in] flags  : flags for the mapping
     */
    virtual gboolean gstBufferMap(GstBuffer *buffer, GstMapInfo *info, GstMapFlags flags) = 0;

    /**
     * @brief Releases the memory previously mapped with gst_buffer_map.
     *
     * @param[in] buffer : a GstBuffer.
     * @param[in] info   : a GstMapInfo
     */
    virtual void gstBufferUnmap(GstBuffer *buffer, GstMapInfo *info) = 0;

    /**
     * @brief Decreases the refcount of the message. If the refcount reaches 0, the message with the associated
     *        metadata and memory will be freed.
     *
     * @param[in] msg : a GstMessage.
     */
    virtual void gstMessageUnref(GstMessage *msg) = 0;

    /**
     * @brief Gets a message fromt the bus
     *
     * @param[in] bus     : bus to pop from
     * @param[in] timeout : timeout to wait
     * @param[in] types   : types of messages which are handled
     *
     * @retval message from the bus
     */
    virtual GstMessage *gstBusTimedPopFiltered(GstBus *bus, GstClockTime timeout, GstMessageType types) = 0;

    /**
     * @brief Gets a message fromt the bus
     *
     * @param[in] bin       : top-level pipeline
     * @param[in] details   : details of the graph
     * @param[in] file_name : base filename of output graph
     */
    virtual void gstDebugBinToDotFileWithTs(GstBin *bin, GstDebugGraphDetails details, const gchar *file_name) = 0;

    /**
     * @brief Retrieves the factory that was used to create this element.
     *
     * @param[in] element   : a GstElement to request the element factory of.
     *
     * @retval the GstElementFactory used for creating this element or NULL if element has not been registered
     *         (static element). no refcounting is needed.
     */
    virtual GstElementFactory *gstElementGetFactory(GstElement *element) const = 0;

    /**
     * @brief Check if factory is of the given types.
     *
     * @param[in] factory : a GstElementFactory
     * @param[in] type    : a GstElementFactoryListType
     *
     * @retval TRUE if factory is of type.
     */
    virtual gboolean gstElementFactoryListIsType(GstElementFactory *factory, GstElementFactoryListType type) const = 0;

    /**
     * @brief Copy caps.
     *
     * Creates a new caps structure and copies the old caps into it. The new caps shall have a refcount
     * of 1 and is owned by the caller.
     *
     * @param[in] caps  : the gstCaps to be copied
     *
     * @retval pointer to the new caps.
     */
    virtual GstCaps *gstCapsCopy(const GstCaps *caps) const = 0;

    /**
     * @brief Sets the fields in the gst caps.
     *
     * @param[in] caps      : caps to set
     * @param[in] field     : the name of the field to set
     * @param[in] ... ...   : Variable arguments, should be in the form field name, field type (as a GType), value(s)
     * and be NULL terminated
     */
    virtual void gstCapsSetSimple(GstCaps *caps, const gchar *field, ...) const = 0;

    /**
     * @brief Gets the buffer timestamps and live status from the Qos message.
     *
     * Timestamps are invalid if set to GST_CLOCK_TIME_NONE.
     *
     * @param[in] message       : a GST_MESSAGE_QOS message.
     * @param[in] live          : whether the message was generated from a live event.
     * @param[in] running_time  : the running time of the buffer that generated the message.
     * @param[in] stream_time   : the stream time of the buffer that generated the message.
     * @param[in] timestamp     : the timestamp of the buffer that generated the message.
     * @param[in] duration      : the duration of the buffer that generated the message.
     */
    virtual void gstMessageParseQos(GstMessage *message, gboolean *live, guint64 *running_time, guint64 *stream_time,
                                    guint64 *timestamp, guint64 *duration) const = 0;

    /**
     * @brief Gets the Qos stats of the current playback period.
     *
     * The dropped and processed frames are invalid if format is set to GST_FORMAT_UNDEFINED.
     * The dropped and processed frames are unknown if they are set to a value of -1.
     *
     * @param[in] message   : a GST_MESSAGE_QOS message.
     * @param[in] format    : the units of both processed and dropped. Video will use GST_FORMAT_BUFFERS and Audio will
     * liekly use GST_FORMAT_DEFAULT.
     * @param[in] processed : the total number of video frames/audio samples processed since state READY or flush.
     * @param[in] dropped   : the total number of video frames/audio samples dropped since state READY or flush.
     */
    virtual void gstMessageParseQosStats(GstMessage *message, GstFormat *format, guint64 *processed,
                                         guint64 *dropped) const = 0;

    /**
     * @brief Gets the metadata with key in klass.
     *
     * @param[in] klass : the klass to get the metadata from.
     * @param[in] key   : the key to use.
     *
     * @retval the metadata for key.
     */
    virtual const gchar *gstElementClassGetMetadata(GstElementClass *klass, const gchar *key) const = 0;

    /**
     * @brief Gets to string of the format.
     *
     * @param[in] format : the format to get the string of.
     *
     * @retval the string of the format or NULL if unknown.
     */
    virtual const gchar *gstFormatGetName(GstFormat format) const = 0;

    /**
     * @brief Allocate a new GstSegment structure and initialize it using gst_segment_init.
     *        Free-function: gst_segment_free
     *
     * @retval a new GstSegment, free with gst_segment_free.
     */
    virtual GstSegment *gstSegmentNew() const = 0;

    /**
     * @brief Initialize segment to its default values.
     *
     * @param[in] segment : a GstSegment structure.
     * @param[in] format  : the format of the segment.
     *
     */
    virtual void gstSegmentInit(GstSegment *segment, GstFormat format) const = 0;

    /**
     * @brief Free the allocated segment segment.
     *
     * @param[in] segment : a GstSegment structure.
     *
     */
    virtual void gstSegmentFree(GstSegment *segment) const = 0;

    /**
     * @brief Create a new SEGMENT event for segment.
     *
     * @param[in] segment : a GstSegment structure.
     *
     * @retval the new SEGMENT event.
     *
     */
    virtual GstEvent *gstEventNewSegment(const GstSegment *segment) const = 0;

    /**
     * @brief Create a new custom-typed event.
     *
     * @param[in] type      : The type of the new event.
     * @param[in] structure : the structure for the event. The event will take ownership of the structure.
     *
     * @retval the new custom event.
     *
     */
    virtual GstEvent *gstEventNewCustom(GstEventType type, GstStructure *structure) const = 0;

    /**
     * @brief Creates a new GstStructure with the given name. Free-function: gst_structure_free
     *
     * @param[in] name       : name of new structure
     * @param[in] firstfield : name of first field to set
     * @param[in] ...        : additional arguments
     *
     * @retval a new GstStructure
     */
    virtual GstStructure *gstStructureNew(const gchar *name, const gchar *firstfield, ...) const = 0;

    /**
     * @brief Initializes writer with the given memory area. If initialized is TRUE it is possible to read size bytes
     *        from the GstByteWriter from the beginning.
     *
     * @param[in] writer      : GstByteWriter instance
     * @param[in] data        : Memory area for writing.
     * @param[in] size        : Size of data in bytes
     * @param[in] initialized : If TRUE the complete data can be read from the beginning
     *
     */
    virtual void gstByteWriterInitWithData(GstByteWriter *writer, guint8 *data, guint size,
                                           gboolean initialized) const = 0;

    /**
     * @brief Writes a unsigned big endian 16 bit integer to writer.
     *
     * @param[in] writer : GstByteWriter instance
     * @param[in] val    : Value to write
     *
     * @retval TRUE if the value could be written
     */
    virtual gboolean gstByteWriterPutUint16Be(GstByteWriter *writer, guint16 val) const = 0;

    /**
     * @brief Writes a unsigned big endian 32 bit integer to writer.
     *
     * @param[in] writer : GstByteWriter instance
     * @param[in] val    : Value to write
     *
     * @retval TRUE if the value could be written
     */
    virtual gboolean gstByteWriterPutUint32Be(GstByteWriter *writer, guint32 val) const = 0;

    /**
     * @brief Creates a new buffer that wraps the given data. The memory will be freed with g_free and will be marked
     *        writable.
     *
     * @param[in] data : data to wrap
     * @param[in] size : allocated size of data
     *
     * @retval a new GstBuffer
     */
    virtual GstBuffer *gstBufferNewWrapped(gpointer data, gsize size) const = 0;

    /**
     * @brief Creates Opus caps from the given Opus header.
     *
     * @param[in] data : a pointer to an Opus header
     * @param[in] size : the size of the header
     *
     * @retval pointer to the new caps
     */
    virtual GstCaps *gstCodecUtilsOpusCreateCapsFromHeader(gconstpointer data, guint size) const = 0;

    /**
     * @brief Checks if the given caps are exactly the same set of caps.
     *
     * @param[in] caps1  : caps
     * @param[in] caps2  : another caps
     *
     * @retval true if caps are strictly equal
     */
    virtual gboolean gstCapsIsStrictlyEqual(const GstCaps *caps1, const GstCaps *caps2) const = 0;

    /**
     * @brief Tries intersecting caps1 and caps2 and reports whether the result would not be empty
     *
     * @param[in] caps1  : caps
     * @param[in] caps2  : another caps
     *
     * @retval true if intersection would be not empty
     */
    virtual gboolean gstCapsCanIntersect(const GstCaps *caps1, const GstCaps *caps2) const = 0;

    /**
     * @brief Converts a GstStaticCaps to a GstCaps.
     *
     * @param[in] staticCaps : static caps to convert
     *
     * @retval true if caps are strictly equal
     */
    virtual GstCaps *gstStaticCapsGet(GstStaticCaps *staticCaps) const = 0;

    /**
     * @brief Get a list of factories that match the given type. Only elements with a rank greater or equal to minrank
     * will be returned. The list of factories is returned by decreasing rank.
     *
     * @param[in] type    : a GstElementFactoryListType
     * @param[in] minrank : a minimum rank of factory
     *
     * @retval a GList of GstElementFactory elements.
     */
    virtual GList *gstElementFactoryListGetElements(GstElementFactoryListType type, GstRank minrank) const = 0;

    /**
     * @brief Gets the GList of GstStaticPadTemplate for this factory.
     *
     * @param[in] factory : a GstElementFactory
     *
     * @retval the static pad templates
     */
    virtual const GList *gstElementFactoryGetStaticPadTemplates(GstElementFactory *factory) const = 0;

    /**
     * @brief Unrefs each member of list, then frees the list.
     *
     * @param[in] list : list of GstPluginFeature
     */
    virtual void gstPluginFeatureListFree(GList *list) const = 0;

    /**
     * @brief Creates a new GstCaps with one GstStructure.
     *
     * @param[in] media_type : the media type of the structure
     * @param[in] fieldname  : first field to set
     *
     * @retval new caps
     */
    virtual GstCaps *gstCapsNewSimple(const char *media_type, const char *fieldname, ...) const = 0;

    /**
     * @brief Creates a new GstCaps with media_type
     *
     * @param[in] media_type : the media type of the structure
     *
     * @retval new caps
     */
    virtual GstCaps *gstCapsNewEmptySimple(const char *media_type) const = 0;

    /**
     * @brief Creates a new empty GstCaps
     *
     * @retval new caps
     */
    virtual GstCaps *gstCapsNewEmpty() const = 0;

    /**
     * @brief Adds the metadata to a buffer for decryption.
     *
     * @param[in] buffer : the gst encrypted buffer to add the protection meta to.
     * @param[in] info   : the information for decryption.
     *
     * @retval the protected metadata, null on failure
     */
    virtual GstProtectionMeta *gstBufferAddProtectionMeta(GstBuffer *buffer, GstStructure *info) const = 0;

    /**
     * @brief Adds metadata for info to buffer using the parameters in params.
     *
     * @param[in] buffer : the gst buffer
     * @param[in] meta   : the GstMetaInfo
     * @param[in] params : the additional params for the info
     *
     * @retval the metadata for the api in info, nullptr if operation fails
     */
    virtual GstMeta *gstBufferAddMeta(GstBuffer *buffer, const GstMetaInfo *info, gpointer params) const = 0;

    /**
     * @brief Gets the metadata for api on buffer.
     *
     * @param[in] buffer : the gst buffer containing meta
     * @param[in] api : the GType of an API
     *
     * @retval the metadata for api on buffer, nullptr if operation fails
     */
    virtual GstMeta *gstBufferGetMeta(GstBuffer *buffer, GType api) const = 0;

    /**
     * @brief Removes the metadata from the buffer.
     *
     * @param[in] buffer : the gst buffer containing meta
     * @param[in] meta : the meta to remove
     *
     * @retval true if removed successfully, false if no meta does not exist in buffer.
     */
    virtual gboolean gstBufferRemoveMeta(GstBuffer *buffer, GstMeta *meta) const = 0;

    /**
     * @brief Gets the uint of the field with name 'fieldname'.
     *
     * @param[in] structure : the gst structure to search
     * @param[in] fieldname : the name of the field
     * @param[in] value : the value of the field
     *
     * @retval true on success, false if 'fieldname' does not exist or is not a uint.
     */
    virtual gboolean gstStructureGetUint64(const GstStructure *structure, const gchar *fieldname,
                                           guint64 *value) const = 0;

    /**
     * @brief Free this structure
     *
     * @param[in] structure : the gst structure to free
     */
    virtual void gstStructureFree(GstStructure *structure) const = 0;

    /**
     * @brief Create a new step event. The purpose of the step event is to instruct a sink to skip amount (expressed in
     * format) of media. It can be used to implement stepping through the video frame by frame or for doing fast trick modes.
     *
     * @param[in] format : the the format of amount
     * @param[in] amount : the amount of data to step
     * @param[in] rate : the step rate
     * @param[in] flush : flushing steps
     * @param[in] intermediate : intermediate steps
     *
     * @retval new event
     */
    virtual GstEvent *gstEventNewStep(GstFormat format, guint64 amount, gdouble rate, gboolean flush,
                                      gboolean intermediate) const = 0;

    /**
     * @brief Returns current volume
     *
     * @param[in] volume : GstStreamVolume that should be used
     * @param[in] format : GstStreamVolumeFormat which should be returned
     *
     * @retval The current stream volume as linear factor
     */
    virtual gdouble gstStreamVolumeGetVolume(GstStreamVolume *volume, GstStreamVolumeFormat format) const = 0;

    /**
     * @brief Sets new volume
     *
     * @param[in] volume : GstStreamVolume that should be used
     * @param[in] format : GstStreamVolumeFormat of val
     * @param[in] val    : Linear volume factor that should be set
     *
     * @retval The current stream volume as linear factor
     */
    virtual void gstStreamVolumeSetVolume(GstStreamVolume *volume, GstStreamVolumeFormat format, gdouble val) const = 0;

    /**
     * @brief Get current mute status of the pipeline
     *
     * @param[in] volume : GstStreamVolume that should be used
     *
     * @retval Returns TRUE if the stream is muted
     */
    virtual gboolean gstStreamVolumeGetMute(GstStreamVolume *volume) const = 0;

    /**
     * @brief Sets mute status of pipeline
     *
     * @param[in] volume : GstStreamVolume that should be used
     * @param[in] mute   : Mute state that should be set
     *
     */
    virtual void gstStreamVolumeSetMute(GstStreamVolume *volume, gboolean mute) const = 0;

    /**
     * @brief Create a new pipeline with given name.
     *
     * @param[in] name  : Name of new pipeline.
     *
     * @retval The new pipeline element.
     */
    virtual GstElement *gstPipelineNew(const gchar *name) const = 0;

    /**
     * @brief Find a GstPluginFeature in the registry.
     *
     * @param[in] registry  : Registry to search.
     * @param[in] name      : Name of feature to lookup.
     *
     * @retval A GstPluginFeature if successful, the refcount is incremented.
     */
    virtual GstPluginFeature *gstRegistryLookupFeature(GstRegistry *registry, const char *name) const = 0;

    /**
     * @brief Get the number of bytes queued in the appsrc.
     *
     * @param[in] appsrc : The app source.
     *
     * @retval The number of bytes queued.
     */
    virtual guint64 gstAppSrcGetCurrentLevelBytes(GstAppSrc *appsrc) const = 0;

    /**
     * @brief Allocate a new flush start event. The flush start event can be sent upstream and downstream and travels
     * out-of-bounds with the dataflow.
     *
     * @retval a new flush start event.
     */
    virtual GstEvent *gstEventNewFlushStart() const = 0;

    /**
     * @brief Allocate a new flush stop event. The flush stop event can be sent upstream and downstream and travels
     * serialized with the dataflow. It is typically sent after sending a FLUSH_START event to make the pads accept data
     * again.
     *
     * @param[in] reset_time : if time should be reset
     *
     * @retval a new flush stop event.
     */
    virtual GstEvent *gstEventNewFlushStop(gboolean reset_time) const = 0;

    /**
     * @brief Returns a suitable channel mask for the number of channels.
     * Should only be used when a channel mask is unknown.
     *
     * @param[in] channels : the number of channels
     *
     * @retval the channel mask or 0 if unknown.
     */
    virtual guint64 gstAudioChannelGetFallbackMask(gint channels) const = 0;

    /**
     * @brief Sets the fields of the gst structure.
     *
     * @param[in] structure  : the structure to add fields too.
     * @param[in] fieldname  : the name of the first field to add.
     * @param[in] ...        : variable arguments, should be in the form field name, field type (as a GType), value(s)
     * and be NULL terminated.
     */
    virtual void gstStructureSet(GstStructure *structure, const gchar *firstname, ...) const = 0;

    /**
     * @brief Gets the error and debug string from the message. Both gerror and debug must be freed by the caller once complete.
     *
     * @param[in]  message  : a gst error message.
     * @param[out] gerror   : pointer to the error value.
     * @param[out] debug    : pointer to the debug string.
     */
    virtual void gstMessageParseError(GstMessage *message, GError **gerror, gchar **debug) const = 0;

    /**
     * @brief Gets an iterator for the bin that contains all the elements with the GST_ELEMENT_FLAG_SINK flag set.
     *
     * @param[in]  bin  : the bin.
     *
     * @retval an iterator of sink elements.
     */
    virtual GstIterator *gstBinIterateSinks(GstBin *bin) const = 0;

    /**
     * @brief Gets the next item from the iterator.
     *
     * @param[in]  it   : the iterator to iterate.
     * @param[out] elem : pointer to hold the next element.
     *
     * @retval GST_ITERATOR_OK if a valid element is found.
     *         GST_ITERATOR_DONE if there are no more elements.
     *         GST_ITERATOR_RESYNC if the element list was concurrently updated.
     *         GST_ITERATOR_ERROR if an error occured.
     */
    virtual GstIteratorResult gstIteratorNext(GstIterator *it, GValue *elem) const = 0;

    /**
     * @brief Free the iterator.
     *
     * @param[in]  it   : the iterator to free.
     */
    virtual void gstIteratorFree(GstIterator *it) const = 0;

    /**
     * @brief Post a message on the elements bus.
     *
     * @param[in] element   : the element posting the message.
     * @param[in] message   : message to post.
     *
     * @retval TRUE if the message was posted, FALSE otherwise.
     */
    virtual gboolean gstElementPostMessage(GstElement *element, GstMessage *message) const = 0;

    /**
     * @brief Create a new warning message.
     *
     * @param[in] src   : the origin of the message.
     * @param[in] error : GError for this message.
     * @param[in] debug : error info string.
     *
     * @retval New warning message.
     */
    virtual GstMessage *gstMessageNewWarning(GstObject *src, GError *error, const gchar *debug) const = 0;

    /**
     * @brief Get the GError and error string from the message.
     *
     * @param[in]  message  : a message of type WARNING.
     * @param[out] gerror   : loction to store the error.
     * @param[out] debug    : loction to store the error string.
     */
    virtual void gstMessageParseWarning(GstMessage *message, GError **gerror, gchar **debug) const = 0;

    /**
     * @brief Finds the structure at index in the caps.
     *
     * @param[in] caps  : a GstCaps.
     * @param[in] index : index in the caps to get the structure.
     *
     * @retval ptr to a structure.
     */
    virtual GstStructure *gstCapsGetStructure(const GstCaps *caps, guint index) const = 0;

    /**
     * @brief Checks if the structure has the passed name
     *
     * @param[in] structure : a GstStructure.
     * @param[in] name      : name which needs to be checked
     *
     * @retval TRUE if the structure has the passed name
     */
    virtual bool gstStructureHasName(const GstStructure *structure, const gchar *name) const = 0;

    /**
     * @brief Sets the name of an object.
     *
     * @param[in] object    : object to name.
     * @param[in] name      : new name of object.
     *
     * @retval TRUE if name could be set.
     */
    virtual gboolean gstObjectSetName(GstObject *object, const gchar *name) const = 0;

    /**
     * @brief Check if the structure contains the given field
     *
     * @param structure  : a GstStructure.
     * @param fieldname  : the name of the field
     *
     * @return TRUE if the structure has the field
     */

    virtual bool gstStructureHasField(const GstStructure *structure, const gchar *fieldname) const = 0;

    /**
     * @brief Update the segment structure with the field values of a seek event (see gst_event_new_seek).
     *
     * @param[in] segment   : a GstSegment structure.
     * @param[in] rate      : the rate of the segment.
     * @param[in] format    : the format of the segment.
     * @param[in] flags     : the segment flags for the segment
     * @param[in] startType : the seek method
     * @param[in] start     : the seek start value
     * @param[in] stopType  : the seek method
     * @param[in] stop      : the seek stop value
     * @param[out] update   : boolean holding whether position was updated.
     *
     * @retval TRUE if the seek could be performed.
     */
    virtual gboolean gstSegmentDoSeek(GstSegment *segment, gdouble rate, GstFormat format, GstSeekFlags flags,
                                      GstSeekType startType, guint64 start, GstSeekType stopType, guint64 stop,
                                      gboolean *update) const = 0;

    /**
     * @brief Create a new GstSample with the provided details.
     *
     * @param[in] buffer  : a GstBuffer, or NULL
     * @param[in] caps    : a GstCaps, or NULL
     * @param[in] segment : a GstSegment, or NULL
     * @param[in] info    : a GstStructure, or NULL
     *
     * @retval the new GstSample. gst_sample_unref after usage.
     */
    virtual GstSample *gstSampleNew(GstBuffer *buffer, GstCaps *caps, const GstSegment *segment,
                                    GstStructure *info) const = 0;

    /**
     * @brief Decreases the refcount of the sample. If the refcount reaches 0, the sample will be freed.
     *
     * @param[in] sample  : a GstSample
     */
    virtual void gstSampleUnref(GstSample *sample) const = 0;

    /**
     * @brief Extract a buffer from the provided sample and adds it to the queue of buffers that the appsrc element will
     * push to its source pad.
     *
     * @param[in] appsrc  : a GstAppSrc
     * @param[in] sample  : a GstSample from which buffer and caps may be extracted
     *
     * @retval GST_FLOW_OK when the buffer was successfully queued. GST_FLOW_FLUSHING when appsrc is not PAUSED or
     * PLAYING. GST_FLOW_EOS when EOS occurred.
     */
    virtual GstFlowReturn gstAppSrcPushSample(GstAppSrc *appsrc, GstSample *sample) const = 0;

    /**
     * @brief Create a new GstContext with the given parameters.
     *
     * @param context_type The type of the context to create.
     * @param persistent Whether the context should be persistent.
     *
     * @return GstContext* The newly created GstContext.
     */
    virtual GstContext *gstContextNew(const gchar *context_type, gboolean persistent) const = 0;

    /**
     * @brief Get a writable structure from a GstContext.
     *
     * @param[in] context The GstContext to get the structure from.
     *
     * @return GstStructure* The writable structure from the context.
     */
    virtual GstStructure *gstContextWritableStructure(GstContext *context) const = 0;

    /**
     * @brief Set the context of a GstElement.
     *
     * @param[in] element The element to set the context for.
     * @param[in] context The context to set.
     */
    virtual void gstElementSetContext(GstElement *element, GstContext *context) const = 0;

    /**
     * @brief Decrease the reference count of a GstContext.
     *
     * @param[in] context The context to unreference.
     */
    virtual void gstContextUnref(GstContext *context) const = 0;

    /**
     * @brief Adds clipping meta to the audio buffer.
     *
     * @param buffer the buffer to add the clipping meta to
     * @param format the GstFormat of start and clippingEnd
     * @param start the amount of audio to clip from start of buffer
     * @param end the amount of audio to clip from end of buffer
     *
     * @return the GstAudioClippingMeta that was added to the buffer
     */
    virtual GstAudioClippingMeta *gstBufferAddAudioClippingMeta(GstBuffer *buffer, GstFormat format, guint64 start,
                                                                guint64 end) const = 0;

    /**
     * @brief Gets a pad of given \a element
     *
     * @param element element from which pad should be returned
     * @param name name of the pad
     *
     * @return given pad or NULL if failed
     */
    virtual GstPad *gstElementGetStaticPad(GstElement *element, const gchar *name) const = 0;

    /**
     * @brief Gives the pointer to the GstPad object of the element.
     *
     * @param element element from which pad should be returned
     *
     * @return given pad or NULL if failed
     */
    virtual GstPad *gstBaseSinkPad(GstElement *element) const = 0;
};

}; // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_GST_WRAPPER_H_
