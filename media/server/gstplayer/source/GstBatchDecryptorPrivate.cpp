/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "GstBatchDecryptorPrivate.h"

GST_DEBUG_CATEGORY_EXTERN(gst_rialto_batch_decryptor_debug_category);
#define GST_CAT_DEFAULT gst_rialto_batch_decryptor_debug_category

namespace firebolt::rialto::server
{
GstRialtoBatchDecryptorPrivate::GstRialtoBatchDecryptorPrivate(
    GstElement *parentElement, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapperFactory> &gstWrapperFactory,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapperFactory> &glibWrapperFactory)
    : GstRialtoDecryptorPrivate(parentElement, gstWrapperFactory, glibWrapperFactory)
{
}

GstRialtoBatchDecryptorPrivate::~GstRialtoBatchDecryptorPrivate()
{
    if (m_sinkCaps)
    {
        m_gstWrapper->gstCapsUnref(m_sinkCaps);
    }
}

void GstRialtoBatchDecryptorPrivate::setSrcPad(GstPad *srcPad)
{
    m_srcPad = srcPad;
}

GstFlowReturn GstRialtoBatchDecryptorPrivate::chainList(GstBufferList *bufferList)
{
    const guint length = m_gstWrapper->gstBufferListLength(bufferList);
    std::vector<GstBuffer *> buffers;
    GstFlowReturn ret = GST_FLOW_OK;

    GST_TRACE_OBJECT(m_decryptorElement, "Received buffer list, length %u", length);

    buffers.reserve(length);
    for (guint i = 0; i < length; ++i)
    {
        buffers.push_back(m_gstWrapper->gstBufferRef(m_gstWrapper->gstBufferListGet(bufferList, i)));
    }
    // Release the list's references, so the buffers become writable
    m_gstWrapper->gstBufferListUnref(bufferList);

    size_t first = 0;
    while ((first < buffers.size()) && (ret == GST_FLOW_OK))
    {
        const size_t count = std::min(kMaxBatchSize, buffers.size() - first);
        ret = processBuffers(std::vector<GstBuffer *>(buffers.begin() + first, buffers.begin() + first + count));
        first += count;
    }
    // Unref buffers not handled due to error
    for (size_t i = first; i < buffers.size(); ++i)
    {
        m_gstWrapper->gstBufferUnref(buffers[i]);
    }
    return ret;
}

gboolean GstRialtoBatchDecryptorPrivate::sinkEvent(GstPad *pad, GstObject *parent, GstEvent *event)
{
    gboolean ret = true;
    GST_TRACE_OBJECT(m_decryptorElement, "Received event: %" GST_PTR_FORMAT, event);

    if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS)
    {
        GstCaps *caps{nullptr};
        m_gstWrapper->gstEventParseCaps(event, &caps);
        // Keep encrypted caps for decryptMulti, as GstRialtoDecryptor does
        m_gstWrapper->gstCapsReplace(&m_sinkCaps, caps);

        ret = m_gstWrapper->gstPadSetCaps(m_srcPad, caps);
        m_gstWrapper->gstEventUnref(event);
    }
    else
    {
        ret = m_gstWrapper->gstPadEventDefault(pad, parent, event);
    }
    return ret;
}

GstFlowReturn GstRialtoBatchDecryptorPrivate::processBuffers(const std::vector<GstBuffer *> &buffers)
{
    GstFlowReturn ret = decryptMulti(buffers, m_sinkCaps);

    size_t pushed = 0;
    if (ret == GST_FLOW_OK)
    {
        while ((pushed < buffers.size()) && (ret == GST_FLOW_OK))
        {
            ret = m_gstWrapper->gstPadPush(m_srcPad, buffers[pushed++]);
        }
    }
    else if (ret == GST_BASE_TRANSFORM_FLOW_DROPPED)
    {
        ret = GST_FLOW_OK;
    }

    for (size_t i = pushed; i < buffers.size(); ++i)
    {
        m_gstWrapper->gstBufferUnref(buffers[i]);
    }
    return ret;
}

GstFlowReturn GstRialtoBatchDecryptorPrivate::decryptMulti(std::vector<GstBuffer *> bufferVec, GstCaps *caps)
{
    GstFlowReturn ret = GST_FLOW_OK;
    bool contentEncrypted{false};
    bool outputRestricted{false};
    int32_t keySessionId{-1};

    for (GstBuffer *buffer : bufferVec)
    {
        GstRialtoProtectionData *protectionData = m_metadataWrapper->getProtectionMetadataData(buffer);
        if (!protectionData)
        {
            GST_TRACE_OBJECT(m_decryptorElement, "Clear sample");
            continue;
        }
        contentEncrypted = true;

        // Create new GstProtectionMeta decrypt
        GstStructure *info = createProtectionMetaInfo(protectionData);
        GstProtectionMeta *meta = m_gstWrapper->gstBufferAddProtectionMeta(buffer, info);
        if (meta == nullptr)
        {
            GST_ERROR_OBJECT(m_decryptorElement, "Failed to add protection meta to the buffer");
            ret = GST_BASE_TRANSFORM_FLOW_DROPPED;
            break;
        }
        if (keySessionId == -1)
        {
            keySessionId = protectionData->keySessionId;
        }
        else if (keySessionId != protectionData->keySessionId)
        {
            GST_ERROR_OBJECT(m_decryptorElement, "Trying to decrypt buffers for different key sessions");
            ret = GST_BASE_TRANSFORM_FLOW_DROPPED;
            break;
        }
    }

    if (contentEncrypted && (ret == GST_FLOW_OK))
    {
        if (m_decryptionService)
        {
            firebolt::rialto::MediaKeyErrorStatus status =
                m_decryptionService->decryptMulti(keySessionId, bufferVec, caps);
            if (firebolt::rialto::MediaKeyErrorStatus::OUTPUT_RESTRICTED == status)
            {
                outputRestricted = true;
                ret = GST_BASE_TRANSFORM_FLOW_DROPPED;
            }
            else if (firebolt::rialto::MediaKeyErrorStatus::OK != status)
            {
                GST_ERROR_OBJECT(m_decryptorElement, "Failed to decrypt the buffers");
                ret = GST_BASE_TRANSFORM_FLOW_DROPPED;
            }
        }
        else
        {
            GST_ERROR_OBJECT(m_decryptorElement, "No decryption service object");
            ret = GST_BASE_TRANSFORM_FLOW_DROPPED;
        }
    }

    for (GstBuffer *buffer : bufferVec)
    {
        m_metadataWrapper->removeProtectionMetadata(buffer);
    }

    if (ret != GST_FLOW_OK)
    {
        if (outputRestricted)
        {
            postHdcpFailure();
        }
        else
        {
            postDecryptWarning("Failed to decrypt buffers, dropping frames and continuing");
        }
    }
    return ret;
}
}; // namespace firebolt::rialto::server
