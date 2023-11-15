/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2019 RDK Management
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

#ifndef __RDK_GSTREAMER_UTILS_H___
#define __RDK_GSTREAMER_UTILS_H___
#include <gst/gst.h>
#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>

namespace rdk_gstreamer_utils {

    #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #define LOG_RGU(fmt, ...) do { fprintf(stderr, "[RGU:%s:%d]: " fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); fflush(stderr); } while (0)
    typedef long long llong;

    enum rgu_Ease
    {
        EaseLinear = 0,
        EaseInCubic,
        EaseOutCubic,
        EaseCount
    };
    enum rgu_gstelement
    {
        GSTELEMENTNULL= 0,
        GSTAUDIODECODER,
        GSTAUDIOPARSER,
        GSTAUDIOQUEUE
    };

    enum MediaType
    {
        MEDIA_UNKNOWN = -1,
        MEDIA_AUDIO = 0,
        MEDIA_VIDEO = 1
    };
    struct rdkGstreamerUtilsPlaybackGrp{
        GstElement *gstPipeline;
        GstElement *curAudioPlaysinkBin;
        GstElement *curAudioDecodeBin;
        GstElement *curAudioDecoder;
        GstElement *curAudioParse;
        GstElement *curAudioTypefind;
        bool     linkTypefindParser;
        bool isAudioAAC;
    };
    struct AudioAttributes {
    AudioAttributes()
        : mNumberOfChannels(0), mSamplesPerSecond(0), mBitrate(0), mBlockAlignment(0), mCodecSpecificData(nullptr), mCodecSpecificDataLen(0)
    {}
    /**
     * "Codecs Parameter" string as in
     * http://tools.ietf.org/search/draft-gellens-mime-bucket-bis-09 as the
     * string is defined in the context of an ISO file. The string will be in
     * the form of a simp-list production defined in
     * http://tools.ietf.org/search/draft-gellens-mime-bucket-bis-09, without
     * the enclosing DQUOTE characters.
     *
     * For example, HE-AAC would be expressed as
     *
     *     mp4a.40.2, mp4a.40.5
     *
     * since the HE profile contains both AAC-LC and SBR. Dolby Digital Plus
     * would be expressed as:
     *
     *      ec-3.A2 or ec-3.A6
     */
    std::string mCodecParam;

    /** Number of channels */
    uint32_t mNumberOfChannels;

    /** Sampling frequency */
    uint32_t mSamplesPerSecond;

    /** Bit rate */
    uint32_t mBitrate;

    /** Block Align */
    uint32_t mBlockAlignment;

    /**
     * Byte array of configuration data for the video encoding.  The format of
     * the data is codec-specific. For HE-AAC, the data mCodecSpecificData is
     * the AudioSpecificConfig data as defined in 14496-3-2009 Sec. 1.6.2.1.
     * The length of the codec specific data is given by mCodecSpecificDataLen;
     */
    const uint8_t *mCodecSpecificData;
    uint32_t mCodecSpecificDataLen;
    };

    inline unsigned getGstPlayFlag(const char* nick)
    {
        static GFlagsClass* flagsClass = static_cast<GFlagsClass*>(g_type_class_ref(g_type_from_name("GstPlayFlags")));

        GFlagsValue* flag = g_flags_get_value_by_nick(flagsClass, nick);
        if (!flag)
            return 0;

        return flag->value;
    }

    bool installUnderflowCallbackFromPlatform(GstElement *pipeline,
        GCallback underflowVideoCallback,
        GCallback underflowAudioCallback,
        gpointer data);

    void initVirtualDisplayHeightandWidthFromPlatform(unsigned int* mVirtualDisplayHeight, unsigned int* mVirtualDisplayWidth);

    bool IntialVolSettingNeeded();
    bool isSocAudioFadeSupported();
    void doAudioEasingonSoc(double target, uint32_t duration, rgu_Ease ease);

    bool isPtsOffsetAdjustmentSupported();
    int getPtsOffsetAdjustment(const std::string& audioCodecString);
    void setVideoProperty(GstElement *pipeline);
    void processAudioGap(GstElement *pipeline,gint64 gapstartpts,gint32 gapduration,gint64 gapdiscontinuity,bool audioaac);
    void enableAudioSwitch(GstElement *pipeline);
    GstElement * configureUIAudioSink(bool TTSenabled);
    bool isUIAudioVGAudioMixSupported();
    std::map<rgu_gstelement,GstElement *> createNewAudioElements(bool isAudioAAC,bool createqueue);
    unsigned getNativeAudioFlag();
    void configAudioCap(AudioAttributes *pAttrib, bool *audioaac, bool svpenabled, GstCaps **appsrcCaps);
    bool performAudioTrackCodecChannelSwitch(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, const void *pSampleAttr, AudioAttributes *pAudioAttr, uint32_t *pStatus, unsigned int *pui32Delay,
                                                 llong *pAudioChangeTargetPts, const llong *pcurrentDispPts, unsigned int *audio_change_stage, GstCaps **appsrcCaps,
                                                 bool *audioaac, bool svpenabled, GstElement *aSrc, bool *ret);
    void setAppSrcParams(GstElement *aSrc,MediaType mediatype);
    void setPixelAspectRatio(GstCaps ** ppCaps,GstCaps *appsrcCaps,uint32_t pixelAspectRatioX,uint32_t pixelAspectRatioY);
    void deepElementAdded(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, GstBin* pipeline, GstBin* bin, GstElement* element);
    void audioMixerGetDeviceInfo(uint32_t& preferredFrames, uint32_t& maximumFrames);
    size_t audioMixerGetBufferDelay(int64_t queuedBytes,int bufferDelayms);
    uint64_t audioMixerGetFifoSize();
    void configVideoCap(std::string vCodec,uint32_t imageWidth,uint32_t imageHeight,uint32_t frameRateValue,uint32_t frameRateScale,bool svpEnabled,gchar **capsString);
    void audioMixerConfigurePipeline(GstElement *gstPipeline,GstElement *aSink,GstElement *aSrc,bool attenuateOutput);
    uint64_t audioMixerGetQueuedBytes(uint64_t bytesPushed,uint64_t bytesPlayed);
    void switchToAudioMasterMode();
    void setVideoSinkMode(GstElement * videoSink);
    void setKeyFrameFlag(GstBuffer *gstBuffer,bool val);
    bool getDelayTimerEnabled();
    void SetAudioServerParam(bool enabled);
} // namespace rdk_gstreamer_utils
#endif /* __RDK_GSTREAMER_UTILS_H___ */
