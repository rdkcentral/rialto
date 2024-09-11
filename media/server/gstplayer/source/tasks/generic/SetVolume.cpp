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

#include "tasks/generic/SetVolume.h"
#include "IRdkGstreamerUtilsWrapper.h"
#include "RialtoServerLogging.h"
#include <cmath>
#include <gst/gst.h>

namespace firebolt::rialto::server::tasks::generic
{
SetVolume::SetVolume(GenericPlayerContext &context, 
                    IGstGenericPlayerPrivate &player, 
                    std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
                    std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper,
                    std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper,
                    double targetVolume, 
                    uint32_t volumeDuration, 
                    firebolt::rialto::EaseType easeType)
    : m_context{context}, m_player{player}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper},
      m_targetVolume{targetVolume}, m_volumeDuration{volumeDuration}, m_easeType{easeType}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetVolume");
}

SetVolume::~SetVolume()
{
    RIALTO_SERVER_LOG_DEBUG("SetVolume finished");
}

void SetVolume::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetVolume");

    if (!m_context.pipeline)
    {
        RIALTO_SERVER_LOG_ERROR("Setting volume failed. Pipeline is NULL");
        return;
    }

    // Get the audio sink from the player
    GstElement *audioSink = m_player.getSink(firebolt::rialto::MediaSourceType::AUDIO);

    // Case 1: Handle GStreamer-based audio fade if the "audio-fade" property exists
    if (audioSink && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(audioSink), "audio-fade"))
    {
        gchar fadeStr[32];
        uint32_t scaledTarget = trunc(100 * m_targetVolume);
        std::string easeString = "L"; 

        switch (m_easeType)
        {
        default:
        case firebolt::rialto::EaseType::EASE_LINEAR:
            easeString = "L";
            RIALTO_SERVER_LOG_DEBUG("Audio Easing function: Ease Linear");
            break;
        case firebolt::rialto::EaseType::EASE_IN_CUBIC:
            easeString = "I";
            RIALTO_SERVER_LOG_DEBUG("Audio Easing function: Ease In Cubic");
            break;
        case firebolt::rialto::EaseType::EASE_OUT_CUBIC:
            easeString = "O";
            RIALTO_SERVER_LOG_DEBUG("Audio Easing function: Ease Out Cubic");
            break;
        }

        snprintf(reinterpret_cast<gchar *>(fadeStr), sizeof(fadeStr), "%u,%u,%s", scaledTarget, m_volumeDuration, easeString.c_str());
        RIALTO_SERVER_LOG_DEBUG("Fade String: %s", fadeStr);

        m_glibWrapper->gObjectSet(audioSink, "audio-fade", fadeStr, nullptr);

    
    }
    else if (m_rdkGstreamerUtilsWrapper->isSocAudioFadeSupported())
    {
        RIALTO_SERVER_LOG_DEBUG("SOC audio fading is supported, applying SOC audio fade");
        m_rdkGstreamerUtilsWrapper->doAudioEasingonSoc(m_targetVolume, m_volumeDuration,
                                                       static_cast<firebolt::rialto::wrappers::rgu_Ease>(m_easeType));
    }    
    else
    {
        // Case 2: If no GStreamer audio-fade property, attempt SOC-based fading
        RIALTO_SERVER_LOG_DEBUG("No audio-fade property found, trying SOC audio easing");
        m_gstWrapper->gstStreamVolumeSetVolume(GST_STREAM_VOLUME(m_context.pipeline), 
                                                GST_STREAM_VOLUME_FORMAT_LINEAR, m_targetVolume);   
    }

    if(audioSink)
        m_gstWrapper->gstObjectUnref(GST_OBJECT(audioSink));
}
} // namespace firebolt::rialto::server::tasks::generic
