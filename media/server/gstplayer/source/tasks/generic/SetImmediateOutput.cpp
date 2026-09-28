/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include "SetImmediateOutput.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
SetImmediateOutput::SetImmediateOutput(GenericPlayerContext &context,
                                       const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                       const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                                       IGstGenericPlayerPrivate &player, const MediaSourceType &type,
                                       bool immediateOutput)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_player(player), m_type{type},
      m_immediateOutput{immediateOutput}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetImmediateOutput");
}

SetImmediateOutput::~SetImmediateOutput()
{
    RIALTO_SERVER_LOG_DEBUG("SetImmediateOutput finished");
}

void SetImmediateOutput::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetImmediateOutput for %s source", common::convertMediaSourceType(m_type));

    GstElement *decoder = m_player.getDecoder(MediaSourceType::AUDIO);
    if (decoder)
    {
        const auto setDecoderBoolPropertyIfExists = [&](const char *property, bool value) {
            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), property))
            {
                gboolean propertyValue{value ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(decoder, property, propertyValue, nullptr);
            }
        };

        const auto setDecoderIntPropertyIfExists = [&](const char *property, gint value) {
            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), property))
            {
                m_glibWrapper->gObjectSet(decoder, property, value, nullptr);
            }
        };

        const std::string decoderName{GST_ELEMENT_NAME(decoder)};
        if (m_glibWrapper->gStrHasPrefix(decoderName.c_str(), "brcmaudiodecoder"))
        {
            setDecoderBoolPropertyIfExists("sync-off", m_immediateOutput);
            setDecoderIntPropertyIfExists("stream_sync_mode", m_immediateOutput ? 1 : 0);
        }

        m_gstWrapper->gstObjectUnref(decoder);
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Pending immediate-output: audio decoder is NULL");
    }

    GstElement *sink = m_player.getSink(MediaSourceType::AUDIO);
    if (sink)
    {
        const auto setBoolPropertyIfExists = [&](const char *property, bool value) {
            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), property))
            {
                gboolean propertyValue{value ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(sink, property, propertyValue, nullptr);
            }
        };

        const auto setIntPropertyIfExists = [&](const char *property, gint value) {
            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), property))
            {
                m_glibWrapper->gObjectSet(sink, property, value, nullptr);
            }
        };

        const std::string sinkName{GST_ELEMENT_NAME(sink)};
        if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "brcmaudiosink"))
        {
            setBoolPropertyIfExists("low-latency", m_immediateOutput);
            setBoolPropertyIfExists("sync", !m_immediateOutput);
        }
        else if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "rtkaudiosink"))
        {
            setBoolPropertyIfExists("media-tunnel", false);
            setBoolPropertyIfExists("audio-service", true);
            setIntPropertyIfExists("lowdelay-sync-mode", m_immediateOutput ? 0 : 1);
            setBoolPropertyIfExists("sync", !m_immediateOutput);
        }
        else if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "amlhalasink"))
        {
            setBoolPropertyIfExists("llp-mode", m_immediateOutput);
            setBoolPropertyIfExists("sync", !m_immediateOutput);
        }
        else if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "mtkaudiosink"))
        {
            setBoolPropertyIfExists("llp-mode", m_immediateOutput);
        }
        else
        {
            RIALTO_SERVER_LOG_WARN("No immediate-output vendor mapping applied for sink '%s'", sinkName.c_str());
        }

        m_gstWrapper->gstObjectUnref(sink);
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Pending immediate-output: audio sink is NULL");
    }

    if (m_type == MediaSourceType::VIDEO)
    {
        m_context.pendingImmediateOutputForVideo = m_immediateOutput;
        if (m_context.pipeline)
        {
            m_player.setImmediateOutput();
        }
        return;
    }

    if (m_type != MediaSourceType::AUDIO)
    {
        RIALTO_SERVER_LOG_ERROR("SetImmediateOutput not currently supported for source type %s",
                                common::convertMediaSourceType(m_type));
    }
}
} // namespace firebolt::rialto::server::tasks::generic
