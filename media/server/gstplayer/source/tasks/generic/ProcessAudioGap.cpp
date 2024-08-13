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

#include "ProcessAudioGap.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::tasks::generic
{
ProcessAudioGap::ProcessAudioGap(
    GenericPlayerContext &context, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper,
    std::int64_t position, std::uint32_t duration, std::int64_t discontinuityGap)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper},
      m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper}, m_position{position}, m_duration{duration},
      m_discontinuityGap{discontinuityGap}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing ProcessAudioGap");
}

ProcessAudioGap::~ProcessAudioGap()
{
    RIALTO_SERVER_LOG_DEBUG("ProcessAudioGap finished");
}

void ProcessAudioGap::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing ProcessAudioGap");
    if (!m_context.pipeline)
    {
        RIALTO_SERVER_LOG_ERROR("Process audio gap failed - pipeline is null");
        return;
    }
    auto audioSourceIt = m_context.streamInfo.find(MediaSourceType::AUDIO);
    if (audioSourceIt == m_context.streamInfo.end())
    {
        RIALTO_SERVER_LOG_ERROR("Process audio gap failed - no audio source attached");
        return;
    }
    GstAppSrc *appSrc{GST_APP_SRC(audioSourceIt->second.appSrc)};
    GstCaps *caps = m_gstWrapper->gstAppSrcGetCaps(appSrc);
    gchar *capsCStr = m_gstWrapper->gstCapsToString(caps);
    const std::string capsStr = std::string(capsCStr);
    m_glibWrapper->gFree(capsCStr);
    m_gstWrapper->gstCapsUnref(caps);
    const bool audioAac{capsStr.find("audio/mpeg") != std::string::npos};
    m_rdkGstreamerUtilsWrapper->processAudioGap(m_context.pipeline, m_position, m_duration, m_discontinuityGap, audioAac);
}
} // namespace firebolt::rialto::server::tasks::generic
