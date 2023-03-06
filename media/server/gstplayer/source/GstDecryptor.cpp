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
#include "GstDecryptorPrivate.h"
#include "GstProtectionMetadataWrapperFactory.h"
#include "RialtoServerLogging.h"

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_RIALTO_DECRYPTOR_TYPE (gst_rialto_decryptor_get_type())
#define GST_RIALTO_DECRYPTOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_RIALTO_DECRYPTOR_TYPE, GstRialtoDecryptor))
#define GST_RIALTO_DECRYPTOR_CLASS(klass)                                                                              \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_RIALTO_DECRYPTOR_TYPE, GstRialtoDecryptorClass))

typedef struct _GstRialtoDecryptor GstRialtoDecryptor;
typedef struct _GstRialtoDecryptorClass GstRialtoDecryptorClass;
typedef struct firebolt::rialto::server::GstRialtoDecryptorPrivate GstRialtoDecryptorPrivate;

GType gst_rialto_decryptor_get_type(void); // NOLINT(build/function_format)

struct _GstRialtoDecryptor
{
    GstBaseTransform parent;
    GstRialtoDecryptorPrivate *priv;
};

struct _GstRialtoDecryptorClass
{
    GstBaseTransformClass parentClass;
};

G_END_DECLS

static GstStaticPadTemplate sinkTemplate =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate srcTemplate =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY(gst_rialto_decryptor_debug_category);
#define GST_CAT_DEFAULT gst_rialto_decryptor_debug_category

#define gst_rialto_decryptor_parent_class parent_class
G_DEFINE_TYPE_WITH_PRIVATE(GstRialtoDecryptor, gst_rialto_decryptor, GST_TYPE_BASE_TRANSFORM);

static void gst_rialto_decryptor_finalize(GObject *);                   // NOLINT(build/function_format)
static GstCaps *gst_rialto_decryptor_transform_caps(GstBaseTransform *, // NOLINT(build/function_format)
                                                    GstPadDirection, GstCaps *, GstCaps *);
static GstFlowReturn gst_rialto_decryptor_transform_ip(GstBaseTransform *base, // NOLINT(build/function_format)
                                                       GstBuffer *buffer);

static const char *toString(const firebolt::rialto::CipherMode &cipherMode)
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

static void gst_rialto_decryptor_class_init(GstRialtoDecryptorClass *klass) // NOLINT(build/function_format)
{
    GST_DEBUG_CATEGORY_INIT(gst_rialto_decryptor_debug_category, "rialtodecryptor", 0, "Decryptor for Rialto");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_rialto_decryptor_finalize);

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_set_static_metadata(element_class, "Rialto Decryptor", GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
                                          "Decryptor for Rialto.",
                                          "Luke Williamson <luke.williamson@sky.uk>\n"
                                          "Adam Czynszak <adam.czynszak@sky.uk>");

    GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_rialto_decryptor_transform_caps);
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_rialto_decryptor_transform_ip);
    base_transform_class->transform_ip_on_passthrough = FALSE;
}

static void gst_rialto_decryptor_init(GstRialtoDecryptor *self) // NOLINT(build/function_format)
{
    GstRialtoDecryptorPrivate *priv =
        reinterpret_cast<GstRialtoDecryptorPrivate *>(gst_rialto_decryptor_get_instance_private(self));
    GstBaseTransform *base = GST_BASE_TRANSFORM(self);

    self->priv = new (priv) GstRialtoDecryptorPrivate(base, firebolt::rialto::server::IGstWrapperFactory::getFactory());

    gst_base_transform_set_in_place(base, TRUE);
    gst_base_transform_set_passthrough(base, FALSE);
    gst_base_transform_set_gap_aware(base, FALSE);
}

static void gst_rialto_decryptor_finalize(GObject *object) // NOLINT(build/function_format)
{
    GstRialtoDecryptor *self = GST_RIALTO_DECRYPTOR(object);
    GstRialtoDecryptorPrivate *priv =
        reinterpret_cast<GstRialtoDecryptorPrivate *>(gst_rialto_decryptor_get_instance_private(self));

    priv->~GstRialtoDecryptorPrivate();

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static GstCaps *gst_rialto_decryptor_transform_caps(GstBaseTransform *base, // NOLINT(build/function_format)
                                                    GstPadDirection direction, GstCaps *caps, GstCaps *filter)
{
    if (direction == GST_PAD_UNKNOWN)
        return nullptr;

    GstRialtoDecryptor *self = GST_RIALTO_DECRYPTOR(base);

    GST_DEBUG_OBJECT(self, "Transform in direction: %s, caps %" GST_PTR_FORMAT ", filter %" GST_PTR_FORMAT,
                     direction == GST_PAD_SINK ? "GST_PAD_SINK" : "GST_PAD_SRC", caps, filter);

    return GST_BASE_TRANSFORM_CLASS(parent_class)->transform_caps(base, direction, caps, filter);
}

static GstFlowReturn gst_rialto_decryptor_transform_ip(GstBaseTransform *base, // NOLINT(build/function_format)
                                                       GstBuffer *buffer)
{
    GstRialtoDecryptor *self = GST_RIALTO_DECRYPTOR(base);
    GstRialtoDecryptorPrivate *priv =
        reinterpret_cast<GstRialtoDecryptorPrivate *>(gst_rialto_decryptor_get_instance_private(self));

    GST_TRACE_OBJECT(self, "Transform in place buf=(%" GST_PTR_FORMAT ")", buffer);

    GstPad *sink_pad = gst_element_get_static_pad(GST_ELEMENT(self), "sink");
    GstCaps *caps = gst_pad_get_current_caps(sink_pad);

    GstFlowReturn result = priv->decrypt(buffer, caps);
    gst_caps_unref(caps);
    gst_object_unref(sink_pad);

    return result;
}

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

GstElement *
GstDecryptorElementFactory::createDecryptorElement(const gchar *name,
                                                   firebolt::rialto::server::IDecryptionService *decryptionService,
                                                   const std::shared_ptr<IGstWrapper> &gstWrapper) const
{
    GstRialtoDecryptor *decrypter = GST_RIALTO_DECRYPTOR(g_object_new(GST_RIALTO_DECRYPTOR_TYPE, name));
    GstRialtoDecryptorPrivate *priv =
        reinterpret_cast<GstRialtoDecryptorPrivate *>(gst_rialto_decryptor_get_instance_private(decrypter));
    std::shared_ptr<firebolt::rialto::server::IGstProtectionMetadataWrapperFactory> metadataFactory =
        firebolt::rialto::server::IGstProtectionMetadataWrapperFactory::createFactory();
    if (priv)
    {
        priv->setDecryptionService(decryptionService);
        priv->setProtectionMetadataWrapper(metadataFactory->createProtectionMetadataWrapper(gstWrapper));
        return GST_ELEMENT(decrypter);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the decryptor element");
        return nullptr;
    }
}

GstRialtoDecryptorPrivate::GstRialtoDecryptorPrivate(GstBaseTransform *parentElement,
                                                     const std::shared_ptr<IGstWrapperFactory> &gstWrapperFactory)
    : m_decryptorElement(parentElement)
{
    if ((!gstWrapperFactory) || (!(m_gstWrapper = gstWrapperFactory->getGstWrapper())))
    {
        throw std::runtime_error("Cannot create GstWrapper");
    }
}

GstFlowReturn GstRialtoDecryptorPrivate::decrypt(GstBuffer *buffer, GstCaps *caps)
{
    GstRialtoDecryptor *self = GST_RIALTO_DECRYPTOR(m_decryptorElement);
    GstRialtoProtectionData *protectionData = m_metadataWrapper->getProtectionMetadataData(buffer);
    if (!protectionData)
    {
        GST_TRACE_OBJECT(self, "Clear sample");
    }
    else
    {
        if (!m_decryptionService)
        {
            GST_ERROR_OBJECT(self, "No decryption service object");
        }
        else
        {
#ifdef RIALTO_ENABLE_DECRYPT_BUFFER
            if (protectionData->cipherMode == firebolt::rialto::CipherMode::CBC1 ||
                protectionData->cipherMode == firebolt::rialto::CipherMode::CENS)
            {
                GST_WARNING_OBJECT(self, "Untested cipher mode '%s'", toString(protectionData->cipherMode));
            }

            if (protectionData->encryptionPatternSet)
            {
                if (protectionData->cipherMode == firebolt::rialto::CipherMode::CENC ||
                    protectionData->cipherMode == firebolt::rialto::CipherMode::CBC1)
                {
                    GST_WARNING_OBJECT(self, "Encryption pattern set for non-pattern cipherMode '%s'",
                                       toString(protectionData->cipherMode));
                }
            }

            // Create new GstProtectionMeta decrypt
            GstStructure *info = createProtectionMetaInfo(protectionData);
            GstProtectionMeta *meta = m_gstWrapper->gstBufferAddProtectionMeta(buffer, info);
            if (meta == nullptr)
            {
                GST_ERROR_OBJECT(self, "Failed to add protection meta to the buffer");
            }
            else
            {
                firebolt::rialto::MediaKeyErrorStatus status =
                    m_decryptionService->decrypt(protectionData->keySessionId, buffer, caps);
                if (firebolt::rialto::MediaKeyErrorStatus::OK != status)
                {
                    GST_ERROR_OBJECT(self, "Failed decrypt the buffer");
                }
                else
                {
                    GST_TRACE_OBJECT(self, "Decryption successful");
                }
            }
#else
            // TODO(RIALTO-127): Remove
            int32_t keySessionId = protectionData->keySessionId;
            uint32_t subsampleCount = protectionData->subsampleCount;
            uint32_t initWithLast15 = protectionData->initWithLast15;
            GstBuffer *key = protectionData->key;
            GstBuffer *iv = protectionData->iv;
            GstBuffer *subsamples = protectionData->subsamples;

            firebolt::rialto::MediaKeyErrorStatus status = m_decryptionService->decrypt(keySessionId, buffer,
                                                                                        subsamples, subsampleCount, iv,
                                                                                        key, initWithLast15, caps);
            if (firebolt::rialto::MediaKeyErrorStatus::OK != status)
            {
                GST_ERROR_OBJECT(self, "Failed decrypt the buffer");
            }
            else
            {
                GST_TRACE_OBJECT(self, "Decryption successful");
            }
#endif
        }

        m_metadataWrapper->removeProtectionMetadata(buffer);
    }

    // pass it through even in case of failed decryption
    return GST_FLOW_OK;
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

void GstRialtoDecryptorPrivate::setDecryptionService(IDecryptionService *decryptionService)
{
    m_decryptionService = decryptionService;
}

void GstRialtoDecryptorPrivate::setProtectionMetadataWrapper(std::unique_ptr<IGstProtectionMetadataWrapper> &&metadataWrapper)
{
    m_metadataWrapper = std::move(metadataWrapper);
}

}; // namespace firebolt::rialto::server
