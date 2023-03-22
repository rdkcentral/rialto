#include "open_cdm_adapter.h"
extern "C" {
OpenCDMError opencdm_gstreamer_session_decrypt(struct OpenCDMSession *session, GstBuffer *buffer, GstBuffer *subSample,
                                               const uint32_t subSampleCount, GstBuffer *IV, GstBuffer *keyID,
                                               uint32_t initWithLast15)
{
    return ERROR_NONE;
}
}