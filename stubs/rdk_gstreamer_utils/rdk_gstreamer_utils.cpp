#include "rdk_gstreamer_utils.h"
namespace rdk_gstreamer_utils
{
bool performAudioTrackCodecChannelSwitch(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup,
                                         const void *pSampleAttr, AudioAttributes *pAudioAttr, uint32_t *pStatus,
                                         unsigned int *pui32Delay, long long *pAudioChangeTargetPts,
                                         const long long *pcurrentDispPts, unsigned int *audio_change_stage,
                                         GstCaps **appsrcCaps, bool *audioaac, bool svpenabled, GstElement *aSrc,
                                         bool *ret)
{
    return true;
}
} // namespace rdk_gstreamer_utils
