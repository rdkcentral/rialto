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

#include "tasks/AttachSource.h"
#include "IGlibWrapper.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
AttachSource::AttachSource(PlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper,
                           std::shared_ptr<IGlibWrapper> glibWrapper, const Source &source)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_attachedSource{source}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing AttachSource");
}

AttachSource::~AttachSource()
{
    RIALTO_SERVER_LOG_DEBUG("AttachSource finished");
}

void AttachSource::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing AttachSource");
    auto elem = m_context.streamInfo.find(m_attachedSource.type);
    if (elem == m_context.streamInfo.end())
    {
        GstElement *appSrc = nullptr;
        if (m_attachedSource.type == MediaSourceType::AUDIO)
        {
            RIALTO_SERVER_LOG_MIL("Adding Audio appsrc");
            appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "audsrc");
        }
        else if (m_attachedSource.type == MediaSourceType::VIDEO)
        {
            RIALTO_SERVER_LOG_MIL("Adding Video appsrc");
            appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "vidsrc");
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Unknown media source type");
            if (m_attachedSource.caps)
                m_gstWrapper->gstCapsUnref(m_attachedSource.caps);
            return;
        }

        m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(appSrc), m_attachedSource.caps);
        m_context.streamInfo.emplace(m_attachedSource.type, appSrc);
    }
    else
    {
        GstCaps *appsrcCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(elem->second));
        if ((!appsrcCaps) || (!m_gstWrapper->gstCapsIsEqual(appsrcCaps, m_attachedSource.caps)))
        {
            gchar *capsStr = m_gstWrapper->gstCapsToString(m_attachedSource.caps);
            RIALTO_SERVER_LOG_MIL("Updating %s appsrc caps to '%s'",
                                  m_attachedSource.type == MediaSourceType::AUDIO ? "Audio" : "Video", capsStr);
            m_glibWrapper->gFree(capsStr);
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(elem->second), m_attachedSource.caps);
        }

        if (appsrcCaps)
            m_gstWrapper->gstCapsUnref(appsrcCaps);
    }

    if (m_attachedSource.caps)
        m_gstWrapper->gstCapsUnref(m_attachedSource.caps);
}
} // namespace firebolt::rialto::server
