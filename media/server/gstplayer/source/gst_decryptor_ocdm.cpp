//
// Copyright 2020 Comcast Cable Communications Management, LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
#include "gst_decryptor_ocdm.h"

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

#include <opencdm/open_cdm.h>

namespace third_party {
namespace starboard {
namespace rdk {
namespace shared {
namespace drm {

namespace {

G_BEGIN_DECLS

#define COBALT_OCDM_DECRYPTOR_TYPE          (cobalt_ocdm_decryptor_get_type())
#define COBALT_OCDM_DECRYPTOR(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), COBALT_OCDM_DECRYPTOR_TYPE, CobaltOcdmDecryptor))
#define COBALT_OCDM_DECRYPTOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), COBALT_OCDM_DECRYPTOR_TYPE, CobaltOcdmDecryptorClass))

typedef struct _CobaltOcdmDecryptor           CobaltOcdmDecryptor;
typedef struct _CobaltOcdmDecryptorClass      CobaltOcdmDecryptorClass;
typedef struct _CobaltOcdmDecryptorPrivate    CobaltOcdmDecryptorPrivate;

GType cobalt_ocdm_decryptor_get_type(void);

struct _CobaltOcdmDecryptor {
  GstBaseTransform parent;
  CobaltOcdmDecryptorPrivate *priv;
};

struct _CobaltOcdmDecryptorClass {
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

GST_DEBUG_CATEGORY(cobalt_ocdm_decryptor_debug_category);
#define GST_CAT_DEFAULT cobalt_ocdm_decryptor_debug_category

struct _CobaltOcdmDecryptorPrivate {
  _CobaltOcdmDecryptorPrivate() {}

  ~_CobaltOcdmDecryptorPrivate() {}

  void SetCachedCaps(GstCaps* caps) {
    gst_caps_replace(&cached_caps_, caps);

    if ( caps ) {
      const GstStructure *s;
      const gchar *media_type;
      s = gst_caps_get_structure (caps, 0);
      media_type = gst_structure_get_name (s);

      is_video_ = g_str_has_prefix(media_type, "video");
    }
  }

  void SetDecrypterService(IDecryptionService &decryptionService)
  {
    m_decryptionService = decryptionService;
  }

  bool IsVideo() const { return is_video_; }

private:
  std::mutex mutex_;
  std::condition_variable condition_ { mutex_ };

  GstCaps*    cached_caps_ { nullptr };
  IDecryptionService &m_decryptionService;

  bool is_video_ { false };
};

#define cobalt_ocdm_decryptor_parent_class parent_class
G_DEFINE_TYPE_WITH_PRIVATE(CobaltOcdmDecryptor, cobalt_ocdm_decryptor, GST_TYPE_BASE_TRANSFORM);

static void cobalt_ocdm_decryptor_finalize(GObject*);
static GstCaps* cobalt_ocdm_decryptor_transform_caps(GstBaseTransform*, GstPadDirection, GstCaps*, GstCaps*);
static GstFlowReturn cobalt_ocdm_decryptor_transform_ip(GstBaseTransform* base, GstBuffer* buffer);

static void cobalt_ocdm_decryptor_class_init(CobaltOcdmDecryptorClass* klass) {
  GST_DEBUG_CATEGORY_INIT(
    cobalt_ocdm_decryptor_debug_category,
    "cobaltocdm", 0, "OCDM Decryptor for Cobalt");

  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->finalize = GST_DEBUG_FUNCPTR(cobalt_ocdm_decryptor_finalize);

  GstElementClass* element_class = GST_ELEMENT_CLASS(klass);
  gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sinkTemplate));
  gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&srcTemplate));
  gst_element_class_set_static_metadata(
    element_class, "OCDM Decryptor.",
    GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
    "Decryptor element for Cobalt.", "Comcast");

  GstBaseTransformClass* base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
  base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(cobalt_ocdm_decryptor_transform_caps);
  base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(cobalt_ocdm_decryptor_transform_ip);
  base_transform_class->transform_ip_on_passthrough = FALSE;
}

static void cobalt_ocdm_decryptor_init(CobaltOcdmDecryptor* self) {
  CobaltOcdmDecryptorPrivate* priv = reinterpret_cast<CobaltOcdmDecryptorPrivate*>(
    cobalt_ocdm_decryptor_get_instance_private(self));
  self->priv = new (priv) CobaltOcdmDecryptorPrivate();

  GstBaseTransform* base = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(base, TRUE);
  gst_base_transform_set_passthrough(base, FALSE);
  gst_base_transform_set_gap_aware(base, FALSE);
}

static void cobalt_ocdm_decryptor_finalize(GObject* object) {
  CobaltOcdmDecryptor* self = COBALT_OCDM_DECRYPTOR(object);
  CobaltOcdmDecryptorPrivate* priv = reinterpret_cast<CobaltOcdmDecryptorPrivate*>(
    cobalt_ocdm_decryptor_get_instance_private(self));

  priv->~CobaltOcdmDecryptorPrivate();

  GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static GstCaps* cobalt_ocdm_decryptor_transform_caps(GstBaseTransform* base, GstPadDirection direction, GstCaps* caps, GstCaps* filter) {
  if (direction == GST_PAD_UNKNOWN)
    return nullptr;

  CobaltOcdmDecryptor* self = COBALT_OCDM_DECRYPTOR(base);
  CobaltOcdmDecryptorPrivate* priv = reinterpret_cast<CobaltOcdmDecryptorPrivate*>(
    cobalt_ocdm_decryptor_get_instance_private(self));

  GST_DEBUG_OBJECT(self, "Transform in direction: %s, caps %" GST_PTR_FORMAT ", filter %" GST_PTR_FORMAT,
                   direction == GST_PAD_SINK ? "GST_PAD_SINK" : "GST_PAD_SRC", caps, filter);

  priv->SetCachedCaps( nullptr );

  return GST_BASE_TRANSFORM_CLASS(parent_class)->transform_caps(base, direction, caps, filter);
}

static GstFlowReturn cobalt_ocdm_decryptor_transform_ip(GstBaseTransform* base, GstBuffer* buffer) {
  CobaltOcdmDecryptor* self = COBALT_OCDM_DECRYPTOR(base);
  CobaltOcdmDecryptorPrivate* priv = reinterpret_cast<CobaltOcdmDecryptorPrivate*>(
    cobalt_ocdm_decryptor_get_instance_private(self));

  GST_TRACE_OBJECT(self, "Transform in place buf=(%" GST_PTR_FORMAT ")", buffer);

  GstProtectionMeta* protection_meta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(buffer));
  if (!protection_meta) {
    GST_TRACE_OBJECT(self, "Clear sample");
    return GST_FLOW_OK;
  }

  GstFlowReturn ret = GST_FLOW_NOT_SUPPORTED;
  GstStructure* info = protection_meta->info;
  GstBuffer* subsamples = nullptr;
  GstBuffer* iv = nullptr;
  GstBuffer* key = nullptr;
  uint32_t subsample_count = 0u;
  uint32_t key_session_id = 0u;

  const GValue* value = nullptr;

  if (!gst_structure_get_uint(info, "key_session_id", &key_session_id) )
  {
    GST_ELEMENT_ERROR (self, STREAM, DECRYPT, ("Decryption failed"), ("Could not get key_session_id"));
    goto exit;
  }

  value = gst_structure_get_value(info, "kid");
  if (!value) {
    GST_ELEMENT_ERROR (self, STREAM, DECRYPT_NOKEY, ("No key ID available for encrypted sample"), (NULL));
    goto exit;
  }
  key = gst_value_get_buffer(value);

  value = gst_structure_get_value(info, "iv");
  if (!value) {
    GST_ELEMENT_ERROR (self, STREAM, DECRYPT_NOKEY, ("Failed to get IV buffer"), (NULL));
    goto exit;
  }
  iv = gst_value_get_buffer(value);

  if (!gst_structure_get_uint(info, "subsample_count", &subsample_count)) {
    GST_ELEMENT_ERROR (self, STREAM, DECRYPT, ("Failed to get subsamples_count"), (NULL));
    goto exit;
  }

  if (subsample_count) {
    value = gst_structure_get_value(info, "subsamples");
    if (!value) {
      GST_ELEMENT_ERROR (self, STREAM, DECRYPT, ("Failed to get subsamples buffer"), (NULL));
      goto exit;
    }
    subsamples = gst_value_get_buffer(value);
  }

  ret = priv->m_decryptionService->decrypt(key_session_id, buffer, subsamples, subsample_count, iv, key);

  GST_TRACE_OBJECT(self, "ret=%s", gst_flow_get_name(ret));

exit:
  gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protection_meta));
  return ret;
}

}  // namespace

GstElement *CreateDecryptorElement(const gchar* name, IDecryptionService &decryptionService) {
  CobaltOcdmDecryptor *decrypter = COBALT_OCDM_DECRYPTOR(g_object_new (COBALT_OCDM_DECRYPTOR_TYPE, name));
  CobaltOcdmDecryptorPrivate* priv = reinterpret_cast<CobaltOcdmDecryptorPrivate*>(
    cobalt_ocdm_decryptor_get_instance_private(self));
  if (priv)
  {
    priv->SetDecrypterService(decryptionService);
  }
  return GST_ELEMENT ( decrypter );
}

}  // namespace drm
}  // namespace shared
}  // namespace rdk
}  // namespace starboard
}  // namespace third_party
