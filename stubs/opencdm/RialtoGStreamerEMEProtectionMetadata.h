/*
 * Copyright (C) 2022 Sky UK
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_RIALTO_PROTECTION_METADATA_GET_TYPE (rialto_eme_protection_metadata_get_type())
#define GST_RIALTO_PROTECTION_METADATA_INFO (rialto_mse_protection_metadata_get_info())

struct _GstRialtoProtectionMetadata
{
    GstMeta parent;
    GstStructure *info;
};

typedef struct _GstRialtoProtectionMetadata GstRialtoProtectionMetadata;

GType rialto_eme_protection_metadata_get_type();
const GstMetaInfo *rialto_mse_protection_metadata_get_info();
GstRialtoProtectionMetadata *rialto_mse_add_protection_metadata(GstBuffer *gstBuffer, GstStructure *info);

G_END_DECLS
