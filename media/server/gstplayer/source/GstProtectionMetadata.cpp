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

#include "GstProtectionMetadata.h"
#include <gst/gstconfig.h>
#include <stdio.h>
#include "RialtoServerLogging.h"

static gboolean rialto_eme_protection_metadata_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    GstRialtoProtectionMetadata *emeta = reinterpret_cast<GstRialtoProtectionMetadata *>(meta);
    GstRialtoProtectionData *data = static_cast<GstRialtoProtectionData *>(params);
    emeta->data = *data;
    //TODO: meta took ownership of gstbuffer, make sure there are no leak

    if (emeta->data.decryptionService)
    {
        emeta->data.decryptionService->incrementSessionIdUsageCounter(emeta->data.keySessionId);
    }

    return true;
}

static gboolean rialto_eme_protection_metadata_free(GstMeta *meta, GstBuffer *buffer)
{
    GstRialtoProtectionMetadata *emeta = reinterpret_cast<GstRialtoProtectionMetadata *>(meta);

    if (emeta->data.key)
    {
        gst_buffer_unref(emeta->data.key);
    }
    if (emeta->data.iv)
    {
        gst_buffer_unref(emeta->data.iv);
    }
    if (emeta->data.subsamples)
    {
        gst_buffer_unref(emeta->data.subsamples);
    }

    if (emeta->data.decryptionService)
    {
        emeta->data.decryptionService->decrementSessionIdUsageCounter(emeta->data.keySessionId);
    }

    return true;
}

GST_EXPORT GType rialto_eme_protection_metadata_get_type()
{
    static volatile GType g_type;
    static const gchar *api_tags[] = {"rialto", "protection", NULL};

    if (g_once_init_enter(&g_type))
    {
        GType _type = gst_meta_api_type_register("GstRialtoProtectionMetadataAPI", api_tags);
        g_once_init_leave(&g_type, _type);
    }
    return g_type;
}

const GstMetaInfo *rialto_mse_protection_metadata_get_info()
{
    static const GstMetaInfo *metainfo = NULL;
    if (g_once_init_enter(&metainfo))
    {
        const GstMetaInfo *gstMeta = gst_meta_register(GST_RIALTO_PROTECTION_METADATA_GET_TYPE,
                                                       "GstRialtoProtectionMetadata", sizeof(GstRialtoProtectionMetadata),
                                                       (GstMetaInitFunction)rialto_eme_protection_metadata_init,
                                                       (GstMetaFreeFunction)rialto_eme_protection_metadata_free, (GstMetaTransformFunction)NULL);

        g_once_init_leave(&metainfo, gstMeta);
    }
    return metainfo;
}

GstRialtoProtectionMetadata *rialto_mse_add_protection_metadata(GstBuffer *gstBuffer, GstRialtoProtectionData &data)
{
    GstRialtoProtectionMetadata *metadata = reinterpret_cast<GstRialtoProtectionMetadata *>(
        gst_buffer_add_meta(gstBuffer, GST_RIALTO_PROTECTION_METADATA_INFO, &data));

    return metadata;
}

int32_t rialto_mse_protection_metadata_get_key_session_id(GstMeta *meta)
{
    GstRialtoProtectionMetadata *emeta = reinterpret_cast<GstRialtoProtectionMetadata *>(meta);
    return emeta->data.keySessionId;
}

uint32_t rialto_mse_protection_metadata_get_subsample_count(GstMeta *meta)
{
    GstRialtoProtectionMetadata *emeta = reinterpret_cast<GstRialtoProtectionMetadata *>(meta);
    return emeta->data.subsampleCount;
}

uint32_t rialto_mse_protection_metadata_get_init_with_last_15(GstMeta *meta)
{
    GstRialtoProtectionMetadata *emeta = reinterpret_cast<GstRialtoProtectionMetadata *>(meta);
    return emeta->data.initWithLast15;
}

GstBuffer *rialto_mse_protection_metadata_get_key(GstMeta *meta)
{
    GstRialtoProtectionMetadata *emeta = reinterpret_cast<GstRialtoProtectionMetadata *>(meta);
    return emeta->data.key;
}

GstBuffer *rialto_mse_protection_metadata_get_iv(GstMeta *meta)
{
    GstRialtoProtectionMetadata *emeta = reinterpret_cast<GstRialtoProtectionMetadata *>(meta);
    return emeta->data.iv;
}

GstBuffer *rialto_mse_protection_metadata_get_subsamples(GstMeta *meta)
{
    GstRialtoProtectionMetadata *emeta = reinterpret_cast<GstRialtoProtectionMetadata *>(meta);
    return emeta->data.subsamples;
}