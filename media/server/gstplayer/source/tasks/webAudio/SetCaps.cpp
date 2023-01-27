/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "tasks/webAudio/SetCaps.h"
#include "IGlibWrapper.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::webaudio
{
namespace
{
class WebAudioCapsBuilder
{
public:
    WebAudioCapsBuilder(std::shared_ptr<IGstWrapper> gstWrapper, std::shared_ptr<IGlibWrapper> glibWrapper)
        : m_gstWrapper(gstWrapper), m_glibWrapper(glibWrapper)
    {
    }
    virtual GstCaps *buildCaps() = 0;

protected:
    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::shared_ptr<IGlibWrapper> m_glibWrapper;
};

class WebAudioPcmCapsBuilder : public WebAudioCapsBuilder
{
public:
    WebAudioPcmCapsBuilder(std::shared_ptr<IGstWrapper> gstWrapper, std::shared_ptr<IGlibWrapper> glibWrapper,
                           const WebAudioPcmConfig &pcmConfig)
        : WebAudioCapsBuilder(gstWrapper, glibWrapper), m_pcmConfig(pcmConfig)
    {
    }

    GstCaps *buildCaps() override
    {
        GstCaps *caps = m_gstWrapper->gstCapsNewEmptySimple("audio/x-raw");
        addFormat(caps);
        m_gstWrapper->gstCapsSetSimple(caps, "channels", G_TYPE_UINT, m_pcmConfig.channels, "layout", G_TYPE_STRING,
                                       "interleaved", "rate", G_TYPE_UINT, m_pcmConfig.rate, nullptr);

        return caps;
    }

protected:
    void addFormat(GstCaps *caps)
    {
        std::string format;

        if (m_pcmConfig.isFloat)
        {
            format += "F";
        }
        else if (m_pcmConfig.isSigned)
        {
            format += "S";
        }
        else
        {
            format += "U";
        }

        format += std::to_string(m_pcmConfig.sampleSize);

        if (m_pcmConfig.isBigEndian)
        {
            format += "BE";
        }
        else
        {
            format += "LE";
        }

        m_gstWrapper->gstCapsSetSimple(caps, "format", G_TYPE_STRING, format.c_str(), nullptr);
    }

    const WebAudioPcmConfig &m_pcmConfig;
};
}; // namespace

SetCaps::SetCaps(WebAudioPlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper,
                 std::shared_ptr<IGlibWrapper> glibWrapper, const std::string &audioMimeType,
                 const WebAudioConfig *config)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_audioMimeType{audioMimeType}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetCaps");

    if (m_audioMimeType == "audio/x-raw")
    {
        m_config.pcm = config->pcm;
    }
}

SetCaps::~SetCaps()
{
    RIALTO_SERVER_LOG_DEBUG("SetCaps finished");
}

void SetCaps::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetCaps");

    GstCaps *caps = createCapsFromMimeType();
    gchar *capsStr = m_gstWrapper->gstCapsToString(caps);
    std::string strCaps = capsStr;
    m_glibWrapper->gFree(capsStr);

    RIALTO_SERVER_LOG_DEBUG("caps str: '%s'", strCaps.c_str());

    GstCaps *appsrcCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(m_context.source));
    if ((!appsrcCaps) || (!m_gstWrapper->gstCapsIsEqual(appsrcCaps, caps)))
    {
        RIALTO_SERVER_LOG_INFO("Updating web audio appsrc caps to '%s'", strCaps.c_str());
        m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(m_context.source), caps);
    }

    if (appsrcCaps)
        m_gstWrapper->gstCapsUnref(appsrcCaps);
    if (caps)
        m_gstWrapper->gstCapsUnref(caps);
}

GstCaps *SetCaps::createCapsFromMimeType() const
{
    std::unique_ptr<WebAudioCapsBuilder> capsBuilder;

    if (m_audioMimeType == "audio/x-raw")
    {
        capsBuilder = std::make_unique<WebAudioPcmCapsBuilder>(m_gstWrapper, m_glibWrapper, m_config.pcm);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Invalid audio mime type %s", m_audioMimeType.c_str());
        return nullptr;
    }

    return capsBuilder->buildCaps();
}
} // namespace firebolt::rialto::server::webaudio
