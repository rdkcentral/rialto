/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "tasks/generic/RemoveSource.h"
#include "RialtoServerLogging.h"

namespace{
void print_linked_elements(GstPad* elementPad, int depth = 0) 
{
    RIALTO_SERVER_LOG_WARN("lukewill: pad - %s", GST_OBJECT_NAME(elementPad));
    GstElement* linked_element = gst_pad_get_parent_element(elementPad);
    GstPad* nextPad;
    if (linked_element)
    {
        RIALTO_SERVER_LOG_WARN("lukewill: %*s%s", depth * 2, "", GST_OBJECT_NAME(linked_element));
        nextPad = gst_element_get_static_pad(linked_element, "src");
        if (!nextPad)
        {
            nextPad = gst_element_get_static_pad(linked_element, "src_1");
            if (!nextPad)
            {
                nextPad = gst_element_get_static_pad(linked_element, "src_0");
            }
        }
        gst_object_unref(linked_element);
    }
    else
    {
        const gchar* peer_pad_name = GST_OBJECT_NAME(elementPad);
        RIALTO_SERVER_LOG_WARN("lukewill: internal_pad - %s", peer_pad_name);
        if (g_str_has_prefix(peer_pad_name, "proxy")) 
        {
            GstPad* proxyPad1 = GST_PAD(gst_proxy_pad_get_internal(GST_PROXY_PAD(elementPad)));
            RIALTO_SERVER_LOG_WARN("lukewill: proxyPad1 - %s", GST_OBJECT_NAME(proxyPad1));
    
            GstPad* peer_pad_2 = gst_pad_get_peer(proxyPad1);
            RIALTO_SERVER_LOG_WARN("lukewill: peer_pad_2 - %s", GST_OBJECT_NAME(peer_pad_2));
    
            nextPad = GST_PAD(gst_proxy_pad_get_internal(GST_PROXY_PAD(peer_pad_2)));
            RIALTO_SERVER_LOG_WARN("lukewill: prxPad_2 - %s", GST_OBJECT_NAME(nextPad));
        }
        else
        {
            return;
        }
    }

    GstPad* peer_pad;
    if (nextPad)
    {
        peer_pad = gst_pad_get_peer(nextPad);
    }

    if(!peer_pad) 
    {
        GstPad* proxyPad1 = GST_PAD(gst_proxy_pad_get_internal(GST_PROXY_PAD(elementPad)));
        RIALTO_SERVER_LOG_WARN("lukewill: proxyPad1 - %s", GST_OBJECT_NAME(proxyPad1));

        peer_pad = gst_pad_get_peer(proxyPad1);
        RIALTO_SERVER_LOG_WARN("lukewill: peer_pad_2 - %s", GST_OBJECT_NAME(peer_pad));
    }

    if (peer_pad)
    {
        RIALTO_SERVER_LOG_WARN("lukewill: peer pad - %s", GST_OBJECT_NAME(peer_pad));
        print_linked_elements(peer_pad, depth + 1);
    }
    else
    {
        return;
    }

    if (linked_element && nextPad && peer_pad)
    {
        gst_pad_unlink(nextPad, peer_pad);
        gboolean result = gst_element_remove_pad(linked_element, nextPad);
        RIALTO_SERVER_LOG_WARN("lukewill: %u", result);
    }
    gst_object_unref(nextPad);
    gst_object_unref(peer_pad);
}
}
namespace firebolt::rialto::server::tasks::generic
{
RemoveSource::RemoveSource(GenericPlayerContext &context, IGstGenericPlayerClient *client,
                           std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
                           const MediaSourceType &type)
    : m_context{context}, m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}, m_type{type}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing RemoveSource");
}

RemoveSource::~RemoveSource()
{
    RIALTO_SERVER_LOG_DEBUG("RemoveSource finished");
}

void RemoveSource::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing RemoveSource");
    if (MediaSourceType::AUDIO != m_type)
    {
        RIALTO_SERVER_LOG_DEBUG("RemoveSource not supported for type != AUDIO");
        return;
    }
    m_context.audioBuffers.clear();
    m_context.audioNeedData = false;
    m_context.audioNeedDataPending = false;
    m_context.audioSourceRemoved = true;
    m_gstPlayerClient->invalidateActiveRequests(m_type);
    GstElement *source{nullptr};
    auto sourceElem = m_context.streamInfo.find(m_type);
    if (sourceElem != m_context.streamInfo.end())
    {
        source = sourceElem->second.appSrc;
    }
    if (!source)
    {
        RIALTO_SERVER_LOG_WARN("failed to flush - source is NULL");
        return;
    }
    GstEvent *flushStart = m_gstWrapper->gstEventNewFlushStart();
    if (!m_gstWrapper->gstElementSendEvent(source, flushStart))
    {
        RIALTO_SERVER_LOG_WARN("failed to send flush-start event");
    }
    GstEvent *flushStop = m_gstWrapper->gstEventNewFlushStop(FALSE);
    if (!m_gstWrapper->gstElementSendEvent(source, flushStop))
    {
        RIALTO_SERVER_LOG_WARN("failed to send flush-stop event");
    }

    RIALTO_SERVER_LOG_WARN("lukewill: pad investigation start");
    GstIterator * iterator = gst_element_iterate_pads(source);
    gboolean done = FALSE;
    GValue value = { 0, };
    GstPad *pad;
    while (!done) {
        switch (gst_iterator_next(iterator, &value)) {
        case GST_ITERATOR_OK:
        {
            pad = GST_PAD(g_value_dup_object(&value));
            gchar* name = gst_pad_get_name(pad);
            RIALTO_SERVER_LOG_WARN("lukewill: found pad %s", name);
            g_free(name);
            g_value_reset (&value);
            break;
        }
        case GST_ITERATOR_RESYNC:
        {
            gst_iterator_resync (iterator);
            break;
        }
        case GST_ITERATOR_ERROR:
        {
            done = TRUE;
            break;
        }
        case GST_ITERATOR_DONE:
        {
            done = TRUE;
            break;
        }
        default:
            break;
        }
    }
    gst_iterator_free (iterator);
    RIALTO_SERVER_LOG_WARN("lukewill: pad investigation finish");
    
    print_linked_elements(gst_element_get_static_pad(source, "src"));

    GstPad *target = gst_element_get_static_pad(source, "src");
    gst_pad_set_active(target, FALSE);
    gst_pad_unlink(target, gst_pad_get_peer(target));
    gboolean result = gst_element_remove_pad(source, target);
    gst_object_unref(target);
    RIALTO_SERVER_LOG_WARN("lukewill: removed pad %u", result);

    gst_element_no_more_pads(source);

    RIALTO_SERVER_LOG_WARN("lukewill: no more pads");
}
} // namespace firebolt::rialto::server::tasks::generic
