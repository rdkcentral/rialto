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

#include "GstDecryptor.h"

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

GstRialtoDecryptorPrivate *gst_rialto_decryptor_get_priv(GstRialtoDecryptor *decryptor) // NOLINT(build/function_format)
{
    return reinterpret_cast<GstRialtoDecryptorPrivate *>(gst_rialto_decryptor_get_instance_private(decryptor));
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
    GstRialtoDecryptorPrivate *priv = gst_rialto_decryptor_get_priv(self);
    GstBaseTransform *base = GST_BASE_TRANSFORM(self);

    self->priv = new (priv)
        GstRialtoDecryptorPrivate(GST_ELEMENT(base), firebolt::rialto::wrappers::IGstWrapperFactory::getFactory(),
                                  firebolt::rialto::wrappers::IGlibWrapperFactory::getFactory());

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
    GstRialtoDecryptorPrivate *priv = gst_rialto_decryptor_get_priv(self);

    GST_TRACE_OBJECT(self, "Transform in place buf=(%" GST_PTR_FORMAT ")", buffer);

    GstPad *sink_pad = gst_element_get_static_pad(GST_ELEMENT(self), "sink");
    GstCaps *caps = gst_pad_get_current_caps(sink_pad);

    GstFlowReturn result = priv->decrypt(buffer, caps);
    gst_caps_unref(caps);
    gst_object_unref(sink_pad);

    return result;
}
