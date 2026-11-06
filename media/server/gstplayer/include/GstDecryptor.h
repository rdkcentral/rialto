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

#ifndef FIREBOLT_RIALTO_SERVER_GST_DECRYPTOR_H_ // NOLINT(build/header_guard)
#define FIREBOLT_RIALTO_SERVER_GST_DECRYPTOR_H_

#include "GstDecryptorPrivate.h"
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
GstRialtoDecryptorPrivate *gst_rialto_decryptor_get_priv(GstRialtoDecryptor *decryptor); // NOLINT(build/function_format)

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

#endif
