/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
 * Copyright 2026 Liberty Global Service B.V.
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

#include "GstDecryptorPrivate.h"
#include "GstProtectionMetadataHelperFactory.h"
#include <stdexcept>

GST_DEBUG_CATEGORY_EXTERN(gst_rialto_decryptor_debug_category);
#define GST_CAT_DEFAULT gst_rialto_decryptor_debug_category

namespace firebolt::rialto::server
{

namespace
{
const char *toString(const firebolt::rialto::CipherMode &cipherMode)
{
    switch (cipherMode)
    {
    case firebolt::rialto::CipherMode::CBCS:
    {
        return "cbcs";
    }
    case firebolt::rialto::CipherMode::CENC:
    {
        return "cenc";
    }
    case firebolt::rialto::CipherMode::CBC1:
    {
        return "cbc1";
    }
    case firebolt::rialto::CipherMode::CENS:
    {
        return "cens";
    }
    case firebolt::rialto::CipherMode::UNKNOWN:
    default:
    {
        return "unknown";
    }
    }
}
} // namespace

GstRialtoDecryptorPrivate::GstRialtoDecryptorPrivate(
    GstElement *parentElement, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapperFactory> &gstWrapperFactory,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapperFactory> &glibWrapperFactory)
    : m_decryptorElement(parentElement), m_decryptionService(nullptr)
{
    if ((!gstWrapperFactory) || (!(m_gstWrapper = gstWrapperFactory->getGstWrapper())))
    {
        throw std::runtime_error("Cannot create GstWrapper");
    }

    if ((!glibWrapperFactory) || (!(m_glibWrapper = glibWrapperFactory->getGlibWrapper())))
    {
        throw std::runtime_error("Cannot create GlibWrapper");
    }
}

GstFlowReturn GstRialtoDecryptorPrivate::decrypt(GstBuffer *buffer, GstCaps *caps)
{
    GstRialtoProtectionData *protectionData = m_metadataWrapper->getProtectionMetadataData(buffer);
    GstFlowReturn returnStatus = GST_BASE_TRANSFORM_FLOW_DROPPED; // By default drop frame on failure
    if (!protectionData)
    {
        GST_TRACE_OBJECT(m_decryptorElement, "Clear sample");
        returnStatus = GST_FLOW_OK;
    }
    else
    {
        if (!m_decryptionService)
        {
            GST_ERROR_OBJECT(m_decryptorElement, "No decryption service object");
        }
        else
        {
            if (protectionData->cipherMode == firebolt::rialto::CipherMode::CBC1 ||
                protectionData->cipherMode == firebolt::rialto::CipherMode::CENS)
            {
                GST_WARNING_OBJECT(m_decryptorElement, "Untested cipher mode '%s'", toString(protectionData->cipherMode));
            }

            if (protectionData->encryptionPatternSet)
            {
                if (protectionData->cipherMode == firebolt::rialto::CipherMode::CENC ||
                    protectionData->cipherMode == firebolt::rialto::CipherMode::CBC1)
                {
                    GST_WARNING_OBJECT(m_decryptorElement, "Encryption pattern set for non-pattern cipherMode '%s'",
                                       toString(protectionData->cipherMode));
                }
            }

            if (protectionData->key && m_decryptionService->isExtendedInterfaceUsed(protectionData->keySessionId))
            {
                GstMapInfo keyMap;
                if (m_gstWrapper->gstBufferMap(protectionData->key, &keyMap, GST_MAP_READ))
                {
                    std::vector<uint8_t> playreadyKey(keyMap.data, keyMap.data + keyMap.size);
                    m_gstWrapper->gstBufferUnmap(protectionData->key, &keyMap);
                    m_decryptionService->selectKeyId(protectionData->keySessionId, playreadyKey);
                    m_gstWrapper->gstBufferUnref(protectionData->key);
                    protectionData->key = m_gstWrapper->gstBufferNew();
                }
                else
                {
                    GST_ERROR_OBJECT(m_decryptorElement, "Failed to map playready key id");
                }
            }

            // Create new GstProtectionMeta decrypt
            GstStructure *info = createProtectionMetaInfo(protectionData);
            GstProtectionMeta *meta = m_gstWrapper->gstBufferAddProtectionMeta(buffer, info);
            if (meta == nullptr)
            {
                GST_ERROR_OBJECT(m_decryptorElement, "Failed to add protection meta to the buffer");
            }
            else
            {
                firebolt::rialto::MediaKeyErrorStatus status =
                    m_decryptionService->decrypt(protectionData->keySessionId, buffer, caps);
                if (firebolt::rialto::MediaKeyErrorStatus::OUTPUT_RESTRICTED == status)
                {
                    m_hdcpOutputRestricted = true;
                    m_metadataWrapper->removeProtectionMetadata(buffer);
                    return GST_BASE_TRANSFORM_FLOW_DROPPED;
                }
                else if (firebolt::rialto::MediaKeyErrorStatus::OK != status)
                {
                    GST_ERROR_OBJECT(m_decryptorElement, "Failed decrypt the buffer");
                    m_hdcpOutputRestricted = false;
                }
                else
                {
                    GST_TRACE_OBJECT(m_decryptorElement, "Decryption successful");
                    returnStatus = GST_FLOW_OK;

                    if (m_hdcpOutputRestricted)
                    {
                        postHdcpFailure();
                        m_hdcpOutputRestricted = false;
                    }
                }
            }
        }

        m_metadataWrapper->removeProtectionMetadata(buffer);
    }

    if (GST_BASE_TRANSFORM_FLOW_DROPPED == returnStatus)
    {
        postDecryptWarning("Failed to decrypt buffer, dropping frame and continuing");
    }
    return returnStatus;
}

GstStructure *GstRialtoDecryptorPrivate::createProtectionMetaInfo(GstRialtoProtectionData *protectionData)
{
    GstStructure *info = m_gstWrapper->gstStructureNew("application/x-cenc", "kid", GST_TYPE_BUFFER,
                                                       protectionData->key, "iv", GST_TYPE_BUFFER, protectionData->iv,
                                                       "subsample_count", G_TYPE_UINT, protectionData->subsampleCount,
                                                       "subsamples", GST_TYPE_BUFFER, protectionData->subsamples,
                                                       "encryption_scheme", G_TYPE_UINT, 0, "init_with_last_15",
                                                       G_TYPE_UINT, protectionData->initWithLast15, "cipher-mode",
                                                       G_TYPE_STRING, toString(protectionData->cipherMode), NULL);

    if (protectionData->encryptionPatternSet)
    {
        m_gstWrapper->gstStructureSet(info, "crypt_byte_block", G_TYPE_UINT, protectionData->crypt, NULL);
        m_gstWrapper->gstStructureSet(info, "skip_byte_block", G_TYPE_UINT, protectionData->skip, NULL);
    }
    return info;
}

void GstRialtoDecryptorPrivate::postDecryptWarning(const std::string &message)
{
    // Notify dropped frames upstream as a non-fatal message
    GError *gError{m_glibWrapper->gErrorNewLiteral(GST_STREAM_ERROR, GST_STREAM_ERROR_DECRYPT, message.c_str())};
    gboolean result =
        m_gstWrapper->gstElementPostMessage(m_decryptorElement,
                                            m_gstWrapper->gstMessageNewWarning(GST_OBJECT_CAST(m_decryptorElement),
                                                                               gError, message.c_str()));
    if (!result)
    {
        GST_WARNING_OBJECT(m_decryptorElement, "Could not post decrypt warning");
    }
    m_glibWrapper->gErrorFree(gError);
}

void GstRialtoDecryptorPrivate::postHdcpFailure()
{
    GST_WARNING_OBJECT(m_decryptorElement, "HDCP output protection failure");
    GstStructure *hdcpFailureMsg = m_gstWrapper->gstStructureNew("HDCPProtectionFailure", "message", G_TYPE_STRING,
                                                                 "HDCP Output Protection Error", NULL);
    gboolean result =
        m_gstWrapper->gstElementPostMessage(m_decryptorElement,
                                            m_gstWrapper->gstMessageNewApplication(GST_OBJECT_CAST(m_decryptorElement),
                                                                                   hdcpFailureMsg));
    if (!result)
    {
        GST_WARNING_OBJECT(m_decryptorElement, "Could not post HDCP failure message");
    }
}

void GstRialtoDecryptorPrivate::setDecryptionService(IDecryptionService *decryptionService)
{
    m_decryptionService = decryptionService;
}

void GstRialtoDecryptorPrivate::setProtectionMetadataWrapper(std::unique_ptr<IGstProtectionMetadataHelper> &&metadataWrapper)
{
    m_metadataWrapper = std::move(metadataWrapper);
}

}; // namespace firebolt::rialto::server
