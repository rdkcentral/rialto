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
#include "IGlibWrapper.h"
#include "IGstGenericPlayerPrivate.h"
#include "IGstWrapper.h"
#include "GenericPlayerContext.h"
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
} // namespace

namespace firebolt::rialto::server
{
SetupElement::SetupElement(GenericPlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper,
                           std::shared_ptr<IGlibWrapper> glibWrapper, IGstGenericPlayerPrivate &player, GstElement *element)
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
    if (m_glibWrapper->gStrHasPrefix(GST_ELEMENT_NAME(m_element), "westerossink"))
    {
        if (m_context.isSecondaryVideo)
        {
            m_player.setWesterossinkSecondaryVideo();
        }
        if (!m_context.pendingGeometry.empty())
        {
            m_player.setWesterossinkRectangle();
        }
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
} // namespace firebolt::rialto::server
