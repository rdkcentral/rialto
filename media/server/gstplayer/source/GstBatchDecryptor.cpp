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

#include "GstBatchDecryptor.h"

static GstStaticPadTemplate sinkTemplate =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate srcTemplate =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY(gst_rialto_batch_decryptor_debug_category);
#define GST_CAT_DEFAULT gst_rialto_batch_decryptor_debug_category

#define gst_rialto_batch_decryptor_parent_class parent_class
G_DEFINE_TYPE_WITH_PRIVATE(GstRialtoBatchDecryptor, gst_rialto_batch_decryptor, GST_TYPE_ELEMENT);

static void gst_rialto_batch_decryptor_finalize(GObject *);                           // NOLINT(build/function_format)
static gboolean gst_rialto_batch_decryptor_sink_event(GstPad *pad, GstObject *parent, // NOLINT(build/function_format)
                                                      GstEvent *event);
static GstFlowReturn gst_rialto_batch_decryptor_chain_list(GstPad *pad, // NOLINT(build/function_format)
                                                           GstObject *parent, GstBufferList *bufList);

GstRialtoBatchDecryptorPrivate *
gst_rialto_batch_decryptor_get_priv(GstRialtoBatchDecryptor *decryptor) // NOLINT(build/function_format)
{
    return reinterpret_cast<GstRialtoBatchDecryptorPrivate *>(gst_rialto_batch_decryptor_get_instance_private(decryptor));
}

static void gst_rialto_batch_decryptor_class_init(GstRialtoBatchDecryptorClass *klass) // NOLINT(build/function_format)
{
    GST_DEBUG_CATEGORY_INIT(gst_rialto_batch_decryptor_debug_category, "rialtobatchdecryptor", 0,
                            "Batch Decryptor for Rialto");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_rialto_batch_decryptor_finalize);

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_set_static_metadata(element_class, "Rialto Batch Decryptor", GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
                                          "Batch Decryptor for Rialto", "Piotr Andrzejewski <piotr.2173652@infosys.com>");
}

static void gst_rialto_batch_decryptor_init(GstRialtoBatchDecryptor *self) // NOLINT(build/function_format)
{
    GstRialtoBatchDecryptorPrivate *priv = gst_rialto_batch_decryptor_get_priv(self);
    self->priv = new (priv)
        GstRialtoBatchDecryptorPrivate(GST_ELEMENT(self), firebolt::rialto::wrappers::IGstWrapperFactory::getFactory(),
                                       firebolt::rialto::wrappers::IGlibWrapperFactory::getFactory());

    GstPad *sinkPad = gst_pad_new_from_static_template(&sinkTemplate, "sink");
    gst_pad_set_event_function(sinkPad, GST_DEBUG_FUNCPTR(gst_rialto_batch_decryptor_sink_event));
    gst_pad_set_chain_list_function(sinkPad, GST_DEBUG_FUNCPTR(gst_rialto_batch_decryptor_chain_list));
    gst_element_add_pad(GST_ELEMENT(self), sinkPad);

    GstPad *srcPad = gst_pad_new_from_static_template(&srcTemplate, "src");
    gst_element_add_pad(GST_ELEMENT(self), srcPad);
    self->priv->setSrcPad(srcPad);
}

static void gst_rialto_batch_decryptor_finalize(GObject *object) // NOLINT(build/function_format)
{
    GstRialtoBatchDecryptor *self = GST_RIALTO_BATCH_DECRYPTOR(object);
    GstRialtoBatchDecryptorPrivate *priv = gst_rialto_batch_decryptor_get_priv(self);

    priv->~GstRialtoBatchDecryptorPrivate();

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static GstFlowReturn gst_rialto_batch_decryptor_chain_list(GstPad *pad, // NOLINT(build/function_format)
                                                           GstObject *parent, GstBufferList *bufList)
{
    return GST_RIALTO_BATCH_DECRYPTOR(parent)->priv->chainList(bufList);
}

static gboolean gst_rialto_batch_decryptor_sink_event(GstPad *pad, GstObject *parent, // NOLINT(build/function_format)
                                                      GstEvent *event)
{
    return GST_RIALTO_BATCH_DECRYPTOR(parent)->priv->sinkEvent(pad, parent, event);
}
