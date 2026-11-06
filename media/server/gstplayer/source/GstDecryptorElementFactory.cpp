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

#include "GstDecryptorElementFactory.h"
#include "GstBatchDecryptor.h"
#include "GstDecryptor.h"
#include "GstProtectionMetadataHelperFactory.h"
#include "RialtoServerLogging.h"
#include <stdexcept>

#include <gst/gst.h>

namespace firebolt::rialto::server
{

std::shared_ptr<IGstDecryptorElementFactory> IGstDecryptorElementFactory::createFactory()
{
    std::shared_ptr<IGstDecryptorElementFactory> factory;

    try
    {
        factory = std::make_shared<GstDecryptorElementFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the decryptor element factory, reason: %s", e.what());
    }

    return factory;
}

GstElement *GstDecryptorElementFactory::createDecryptorElement(
    const gchar *name, firebolt::rialto::server::IDecryptionService *decryptionService,
    const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper, bool batchDecryption) const
{
    GstRialtoDecryptorPrivate *priv{};
    GstElement *element{};

    if (batchDecryption)
    {
        GstRialtoBatchDecryptor *decryptor =
            GST_RIALTO_BATCH_DECRYPTOR(g_object_new(GST_RIALTO_BATCH_DECRYPTOR_TYPE, nullptr));
        priv = (decryptor) ? gst_rialto_batch_decryptor_get_priv(decryptor) : nullptr;
        element = (decryptor) ? GST_ELEMENT(decryptor) : nullptr;
    }
    else
    {
        GstRialtoDecryptor *decryptor = GST_RIALTO_DECRYPTOR(g_object_new(GST_RIALTO_DECRYPTOR_TYPE, nullptr));
        priv = (decryptor) ? gst_rialto_decryptor_get_priv(decryptor) : nullptr;
        element = (decryptor) ? GST_ELEMENT(decryptor) : nullptr;
    }

    if (name)
    {
        if (!gstWrapper->gstObjectSetName(GST_OBJECT(element), name))
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set the decryptor name to %s", name);
            g_object_unref(GST_OBJECT(element));
            return nullptr;
        }
    }

    std::shared_ptr<firebolt::rialto::server::IGstProtectionMetadataHelperFactory> metadataFactory =
        firebolt::rialto::server::IGstProtectionMetadataHelperFactory::createFactory();
    if (priv)
    {
        priv->setDecryptionService(decryptionService);
        priv->setProtectionMetadataWrapper(metadataFactory->createProtectionMetadataWrapper(gstWrapper));
        return element;
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the decryptor element");
        g_object_unref(GST_OBJECT(element));
        return nullptr;
    }
}

}; // namespace firebolt::rialto::server
