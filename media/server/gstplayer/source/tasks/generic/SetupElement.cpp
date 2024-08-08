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

#include "tasks/generic/SetupElement.h"
#include "GenericPlayerContext.h"
#include "IGlibWrapper.h"
#include "IGstGenericPlayerPrivate.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"
#include "Utils.h"

namespace
{
/**
 * @brief Callback for audio underflow event from sink. Called by the Gstreamer thread.
 *
 * @param[in] object     : the object that emitted the signal
 * @param[in] fifoDepth  : the fifo depth (may be 0)
 * @param[in] queueDepth : the queue depth (may be NULL)
 * @param[in] self       : The pointer to IGstGenericPlayerPrivate
 *
 * @retval true if the handling of the message is successful, false otherwise.
 */
void audioUnderflowCallback(GstElement *object, guint fifoDepth, gpointer queueDepth, gpointer self)
{
    firebolt::rialto::server::IGstGenericPlayerPrivate *player =
        static_cast<firebolt::rialto::server::IGstGenericPlayerPrivate *>(self);
    player->scheduleAudioUnderflow();
}

/**
 * @brief Callback for video underflow event from sink. Called by the Gstreamer thread.
 *
 * @param[in] object     : the object that emitted the signal
 * @param[in] fifoDepth  : the fifo depth (may be 0)
 * @param[in] queueDepth : the queue depth (may be NULL)
 * @param[in] self       : The pointer to IGstGenericPlayerPrivate
 *
 * @retval true if the handling of the message is successful, false otherwise.
 */
void videoUnderflowCallback(GstElement *object, guint fifoDepth, gpointer queueDepth, gpointer self)
{
    firebolt::rialto::server::IGstGenericPlayerPrivate *player =
        static_cast<firebolt::rialto::server::IGstGenericPlayerPrivate *>(self);
    player->scheduleVideoUnderflow();
}

/**
 * @brief Callback for a autovideosink when a child has been added to the sink.
 *
 * @param[in] obj        : the parent element (autovideosink)
 * @param[in] object     : the child element
 * @param[in] name       : the name of the child element
 * @param[in] self       : The pointer to IGstGenericPlayerPrivate
 */
void autoVideoSinkChildAddedCallback(GstChildProxy *obj, GObject *object, gchar *name, gpointer self)
{
    RIALTO_SERVER_LOG_DEBUG("AutoVideoSink added element %s", name);
    firebolt::rialto::server::IGstGenericPlayerPrivate *player =
        static_cast<firebolt::rialto::server::IGstGenericPlayerPrivate *>(self);
    player->addAutoVideoSinkChild(object);
}

/**
 * @brief Callback for a autovideosink when a child has been removed from the sink.
 *
 * @param[in] obj        : the parent element (autovideosink)
 * @param[in] object     : the child element
 * @param[in] name       : the name of the child element
 * @param[in] self       : The pointer to IGstGenericPlayerPrivate
 */
void autoVideoSinkChildRemovedCallback(GstChildProxy *obj, GObject *object, gchar *name, gpointer self)
{
    RIALTO_SERVER_LOG_DEBUG("AutoVideoSink removed element %s", name);
    firebolt::rialto::server::IGstGenericPlayerPrivate *player =
        static_cast<firebolt::rialto::server::IGstGenericPlayerPrivate *>(self);
    player->removeAutoVideoSinkChild(object);
}
} // namespace

namespace firebolt::rialto::server::tasks::generic
{
SetupElement::SetupElement(GenericPlayerContext &context,
                           std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
                           std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper,
                           IGstGenericPlayerPrivate &player, GstElement *element)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_player{player}, m_element{element}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetupElement");
}

SetupElement::~SetupElement()
{
    RIALTO_SERVER_LOG_DEBUG("SetupElement finished");
}

void SetupElement::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetupElement");

    const std::string kElementTypeName = m_glibWrapper->gTypeName(G_OBJECT_TYPE(m_element));
    if (kElementTypeName == "GstAutoVideoSink")
    {
        // Check and store child sink so we can set underlying properties
        m_glibWrapper->gSignalConnect(m_element, "child-added", G_CALLBACK(autoVideoSinkChildAddedCallback), &m_player);
        m_glibWrapper->gSignalConnect(m_element, "child-removed", G_CALLBACK(autoVideoSinkChildRemovedCallback),
                                      &m_player);

        // AutoVideoSink sets child before it is setup on the pipeline, so check for children here
        GstIterator *sinks = m_gstWrapper->gstBinIterateSinks(GST_BIN(m_element));
        if (sinks && sinks->size > 1)
        {
            RIALTO_SERVER_LOG_WARN("More than one child sink attached");
        }

        GValue elem = G_VALUE_INIT;
        if (m_gstWrapper->gstIteratorNext(sinks, &elem) == GST_ITERATOR_OK)
        {
            m_player.addAutoVideoSinkChild(G_OBJECT(m_glibWrapper->gValueGetObject(&elem)));
        }
        m_glibWrapper->gValueUnset(&elem);

        if (sinks)
            m_gstWrapper->gstIteratorFree(sinks);
    }

    GstElementFactory *elementFactory = m_gstWrapper->gstElementGetFactory(m_element);
    if (elementFactory &&
        m_gstWrapper->gstElementFactoryListIsType(elementFactory,
                                                  GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
    {
        if (!m_context.pendingGeometry.empty())
        {
            m_player.setVideoSinkRectangle();
        }
    }

    if (m_glibWrapper->gStrHasPrefix(GST_ELEMENT_NAME(m_element), "amlhalasink"))
    {
        // Wait for video so that the audio aligns at the starting point with timeout of 4000ms.
        m_glibWrapper->gObjectSet(m_element, "wait-video", TRUE, "a-wait-timeout", 4000, nullptr);

        // Xrun occasionally pauses the underlying sink due to unstable playback, but the rest of the pipeline
        // remains in the playing state. This causes problems with the synchronization of gst element and rialto
        // ultimately hangs waiting for pipeline termination.
        m_glibWrapper->gObjectSet(m_element, "disable-xrun", TRUE, nullptr);
    }

    if (m_glibWrapper->gStrHasPrefix(GST_ELEMENT_NAME(m_element), "brcmaudiosink"))
    {
        m_glibWrapper->gObjectSet(m_element, "async", TRUE, nullptr);
    }

    if (isVideoDecoder(*m_gstWrapper, m_element))
    {
        std::string underflowSignalName = getUnderflowSignalName(*m_glibWrapper, m_element);
        if (!underflowSignalName.empty())
        {
            m_glibWrapper->gSignalConnect(m_element, underflowSignalName.c_str(), G_CALLBACK(videoUnderflowCallback),
                                          &m_player);
        }
    }
    else if (isAudioDecoder(*m_gstWrapper, m_element))
    {
        std::string underflowSignalName = getUnderflowSignalName(*m_glibWrapper, m_element);
        if (!underflowSignalName.empty())
        {
            m_glibWrapper->gSignalConnect(m_element, underflowSignalName.c_str(), G_CALLBACK(audioUnderflowCallback),
                                          &m_player);
        }
    }

    m_gstWrapper->gstObjectUnref(m_element);
}
} // namespace firebolt::rialto::server::tasks::generic
