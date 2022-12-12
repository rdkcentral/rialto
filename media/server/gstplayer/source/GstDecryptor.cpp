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
#include "RialtoServerLogging.h"

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_RIALTO_DECRYPTOR_TYPE          (gst_rialto_decryptor_get_type())
#define GST_RIALTO_DECRYPTOR(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_RIALTO_DECRYPTOR_TYPE, GstRialtoDecryptor))
#define GST_RIALTO_DECRYPTOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), GST_RIALTO_DECRYPTOR_TYPE, GstRialtoDecryptorClass))

typedef struct _GstRialtoDecryptor           GstRialtoDecryptor;
typedef struct _GstRialtoDecryptorClass      GstRialtoDecryptorClass;
typedef struct firebolt::rialto::server::GstRialtoDecryptorPrivate GstRialtoDecryptorPrivate;

GType gst_rialto_decryptor_get_type(void);

struct _GstRialtoDecryptor {
    GstBaseTransform parent;
    GstRialtoDecryptorPrivate *priv;
};

struct _GstRialtoDecryptorClass {
    GstBaseTransformClass parentClass;
};

G_END_DECLS

static GstStaticPadTemplate sinkTemplate =
    GST_STATIC_PAD_TEMPLATE(
        "sink",
        GST_PAD_SINK,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate srcTemplate =
    GST_STATIC_PAD_TEMPLATE(
        "src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY(gst_rialto_decryptor_debug_category);
#define GST_CAT_DEFAULT gst_rialto_decryptor_debug_category

#define gst_rialto_decryptor_parent_class parent_class
G_DEFINE_TYPE_WITH_PRIVATE(GstRialtoDecryptor, gst_rialto_decryptor, GST_TYPE_BASE_TRANSFORM);

static void gst_rialto_decryptor_finalize(GObject*);
static GstCaps* gst_rialto_decryptor_transform_caps(GstBaseTransform*, GstPadDirection, GstCaps*, GstCaps*);
static GstFlowReturn gst_rialto_decryptor_transform_ip(GstBaseTransform* base, GstBuffer* buffer);

static void gst_rialto_decryptor_class_init(GstRialtoDecryptorClass* klass) {
    GST_DEBUG_CATEGORY_INIT(
        gst_rialto_decryptor_debug_category,
        "rialtodecryptor", 0, "Decryptor for Rialto");

    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_rialto_decryptor_finalize);

    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_set_static_metadata(
        element_class, "Rialto Decryptor",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decryptor for Rialto.", "Sky");

    GstBaseTransformClass* base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_rialto_decryptor_transform_caps);
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_rialto_decryptor_transform_ip);
    base_transform_class->transform_ip_on_passthrough = FALSE;
}

static void gst_rialto_decryptor_init(GstRialtoDecryptor* self) {
    GstRialtoDecryptorPrivate* priv = reinterpret_cast<GstRialtoDecryptorPrivate*>(
        gst_rialto_decryptor_get_instance_private(self));
    GstBaseTransform* base = GST_BASE_TRANSFORM(self);

    self->priv = new (priv) GstRialtoDecryptorPrivate(base, firebolt::rialto::server::IGstWrapperFactory::getFactory());

    gst_base_transform_set_in_place(base, TRUE);
    gst_base_transform_set_passthrough(base, FALSE);
    gst_base_transform_set_gap_aware(base, FALSE);
}

static void gst_rialto_decryptor_finalize(GObject* object) {
    GstRialtoDecryptor* self = GST_RIALTO_DECRYPTOR(object);
    GstRialtoDecryptorPrivate* priv = reinterpret_cast<GstRialtoDecryptorPrivate*>(
        gst_rialto_decryptor_get_instance_private(self));

    priv->~GstRialtoDecryptorPrivate();

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static GstCaps* gst_rialto_decryptor_transform_caps(GstBaseTransform* base, GstPadDirection direction, GstCaps* caps, GstCaps* filter) {
    if (direction == GST_PAD_UNKNOWN)
        return nullptr;

    GstRialtoDecryptor* self = GST_RIALTO_DECRYPTOR(base);

    GST_DEBUG_OBJECT(self, "Transform in direction: %s, caps %" GST_PTR_FORMAT ", filter %" GST_PTR_FORMAT,
                    direction == GST_PAD_SINK ? "GST_PAD_SINK" : "GST_PAD_SRC", caps, filter);

    return GST_BASE_TRANSFORM_CLASS(parent_class)->transform_caps(base, direction, caps, filter);
}

static GstFlowReturn gst_rialto_decryptor_transform_ip(GstBaseTransform* base, GstBuffer* buffer) {
    GstRialtoDecryptor* self = GST_RIALTO_DECRYPTOR(base);
    GstRialtoDecryptorPrivate* priv = reinterpret_cast<GstRialtoDecryptorPrivate*>(
        gst_rialto_decryptor_get_instance_private(self));

    GST_TRACE_OBJECT(self, "Transform in place buf=(%" GST_PTR_FORMAT ")", buffer);

    return (priv->decrypt(buffer));
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

GstElement *GstDecryptorElementFactory::createDecryptorElement(const gchar* name, firebolt::rialto::server::IDecryptionService *decryptionService) const
{
    GstRialtoDecryptor *decrypter = GST_RIALTO_DECRYPTOR(g_object_new (GST_RIALTO_DECRYPTOR_TYPE, name));
    GstRialtoDecryptorPrivate* priv = reinterpret_cast<GstRialtoDecryptorPrivate*>(
        gst_rialto_decryptor_get_instance_private(decrypter));
    if (priv)
    {
        priv->setDecryptionService(decryptionService);
        return GST_ELEMENT ( decrypter );
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the decryptor element");
        return nullptr;
    }
}

GstRialtoDecryptorPrivate::GstRialtoDecryptorPrivate(GstBaseTransform* parentElement, const std::shared_ptr<IGstWrapperFactory> &gstWrapperFactory)
    : m_decryptorElement(parentElement)
{
    if ((!gstWrapperFactory) || (!(m_gstWrapper = gstWrapperFactory->getGstWrapper())))
    {
        throw std::runtime_error("Cannot create GstWrapper");
    }
}

GstFlowReturn GstRialtoDecryptorPrivate::decrypt(GstBuffer* buffer)
{
    GstFlowReturn ret = GST_FLOW_ERROR;
    GstRialtoDecryptor* self = GST_RIALTO_DECRYPTOR(m_decryptorElement);

    GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(m_gstWrapper->gstBufferGetProtectionMeta(buffer));
    if (!protectionMeta)
    {
        GST_TRACE_OBJECT(self, "Clear sample");
        ret = GST_FLOW_OK;
    }
    else
    {
        uint32_t keySessionId = 0;
        uint32_t subsampleCount = 0;
        uint32_t initWithLast15 = 0;
        GstBuffer* key = nullptr;
        GstBuffer* iv = nullptr;
        GstBuffer* subsamples = nullptr;

        if (!m_decryptionService)
        {
            GST_ERROR_OBJECT(self, "No decryption service object");
        }
        else if (GST_FLOW_OK != extractDecryptionData(protectionMeta->info, keySessionId, subsampleCount, initWithLast15, &key, &iv, &subsamples))
        {
            GST_ERROR_OBJECT(self, "Extraction of decryption data from the protection meta failed");
        }
        else
        {
            firebolt::rialto::MediaKeyErrorStatus status = m_decryptionService->decrypt(keySessionId, buffer, subsamples, subsampleCount, iv, key, initWithLast15);
            if (firebolt::rialto::MediaKeyErrorStatus::OK != status)
            {
                GST_ERROR_OBJECT(self, "Failed decrypt the buffer");
            }
            else
            {
                GST_TRACE_OBJECT(self, "Decryption successful");
                ret = GST_FLOW_OK;
            }
        }

        m_gstWrapper->gstBufferRemoveMeta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
    }

    return ret;
}

GstFlowReturn GstRialtoDecryptorPrivate::extractDecryptionData(GstStructure* protectionMetaInfo, uint32_t &keySessionId, uint32_t &subsampleCount, uint32_t &initWithLast15, GstBuffer** key, GstBuffer** iv, GstBuffer** subsamples)
{
    GstFlowReturn ret = GST_FLOW_ERROR;
    GstRialtoDecryptor* self = GST_RIALTO_DECRYPTOR(m_decryptorElement);
    const GValue* keyValue = nullptr;
    const GValue* ivValue = nullptr;
    const GValue* subsamplesValue = nullptr;

    if (!m_gstWrapper->gstStructureGetUint(protectionMetaInfo, "key_session_id", &keySessionId) )
    {
        GST_ERROR_OBJECT(self, "Failed to get the key_session_id");
    }
    else if (!m_gstWrapper->gstStructureGetUint(protectionMetaInfo, "subsample_count", &subsampleCount))
    {
        GST_ERROR_OBJECT(self, "Failed to get subsamples_count");
    }
    else if (!m_gstWrapper->gstStructureGetUint(protectionMetaInfo, "init_with_last_15", &initWithLast15))
    {
        GST_ERROR_OBJECT(self, "Failed to get init_with_last_15");
    }
    else if (!(keyValue = m_gstWrapper->gstStructureGetValue(protectionMetaInfo, "kid")))
    {
        GST_ERROR_OBJECT(self, "Failed to get the key ID");
    }
    else if (!(ivValue = m_gstWrapper->gstStructureGetValue(protectionMetaInfo, "iv")))
    {
        GST_ERROR_OBJECT(self, "Failed to get IV buffer");
    }
    else if ((0u != subsampleCount) && !(subsamplesValue = m_gstWrapper->gstStructureGetValue(protectionMetaInfo, "subsamples")))
    {
        GST_ERROR_OBJECT(self, "Failed to get subsamples buffer");
    }
    else if (!(*key = m_gstWrapper->gstValueGetBuffer(keyValue)))
    {
        GST_ERROR_OBJECT(self, "Failed to extract key from GValue");
    }
    else if (!(*iv = m_gstWrapper->gstValueGetBuffer(ivValue)))
    {
        GST_ERROR_OBJECT(self, "Failed to extract iv from GValue");
    }
    else if ((subsamplesValue) && !(*subsamples = m_gstWrapper->gstValueGetBuffer(subsamplesValue)))
    {
        GST_ERROR_OBJECT(self, "Failed to extract subsamples from GValue");
    }
    else
    {
        GST_TRACE_OBJECT(self, "Successfully extracted the decryption info");
        ret = GST_FLOW_OK;
    }

    return ret;
}

void GstRialtoDecryptorPrivate::setDecryptionService(IDecryptionService* decryptionService)
{
    m_decryptionService = decryptionService;
}

}; // namespace firebolt::rialto::server
