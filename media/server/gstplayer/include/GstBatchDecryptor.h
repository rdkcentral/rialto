/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_GST_BATCH_DECRYPTOR_H_ // NOLINT(build/header_guard)
#define FIREBOLT_RIALTO_SERVER_GST_BATCH_DECRYPTOR_H_

#include "GstBatchDecryptorPrivate.h"
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_RIALTO_BATCH_DECRYPTOR_TYPE (gst_rialto_batch_decryptor_get_type())
#define GST_RIALTO_BATCH_DECRYPTOR(obj)                                                                                \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_RIALTO_BATCH_DECRYPTOR_TYPE, GstRialtoBatchDecryptor))
#define GST_RIALTO_BATCH_DECRYPTOR_CLASS(klass)                                                                        \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_RIALTO_BATCH_DECRYPTOR_TYPE, GstRialtoBatchDecryptorClass))

typedef struct _GstRialtoBatchDecryptor GstRialtoBatchDecryptor;
typedef struct _GstRialtoBatchDecryptorClass GstRialtoBatchDecryptorClass;
typedef class firebolt::rialto::server::GstRialtoBatchDecryptorPrivate GstRialtoBatchDecryptorPrivate;

GType gst_rialto_batch_decryptor_get_type(void); // NOLINT(build/function_format)
GstRialtoBatchDecryptorPrivate *
gst_rialto_batch_decryptor_get_priv(GstRialtoBatchDecryptor *decryptor); // NOLINT(build/function_format)

struct _GstRialtoBatchDecryptor
{
    GstElement element;
    GstRialtoBatchDecryptorPrivate *priv;
};

struct _GstRialtoBatchDecryptorClass
{
    GstElementClass parentClass;
};

G_END_DECLS

#endif // FIREBOLT_RIALTO_SERVER_GST_BATCH_DECRYPTOR_H_
