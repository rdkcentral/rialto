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

#include "tasks/generic/AttachSource.h"
#include "GstMimeMapping.h"
#include "IGlibWrapper.h"
#include "IGstWrapper.h"
#include "IMediaPipeline.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"
#include "Utils.h"
#include <unordered_map>

namespace firebolt::rialto::server::tasks::generic
{
AttachSource::AttachSource(GenericPlayerContext &context,
                           const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                           const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                           const std::shared_ptr<IGstTextTrackSinkFactory> &gstTextTrackSinkFactory,
                           IGstGenericPlayerPrivate &player, const std::unique_ptr<IMediaPipeline::MediaSource> &source)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper},
      m_gstTextTrackSinkFactory{gstTextTrackSinkFactory}, m_player{player}, m_attachedSource{source->copy()}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing AttachSource");
}

AttachSource::~AttachSource()
{
    RIALTO_SERVER_LOG_DEBUG("AttachSource finished");
}

void AttachSource::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing AttachSource %u", static_cast<uint32_t>(m_attachedSource->getType()));

    if (m_attachedSource->getType() == MediaSourceType::UNKNOWN)
    {
        RIALTO_SERVER_LOG_ERROR("Unknown media source type");
        return;
    }

    if (m_context.streamInfo.find(m_attachedSource->getType()) == m_context.streamInfo.end())
    {
        addSource();
    }
    else if (m_attachedSource->getType() == MediaSourceType::AUDIO)
    {
        reattachAudioSource();
    }
    else if (m_attachedSource->getType() == MediaSourceType::SUBTITLE)
    {
        reattachSubtitleSource();
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("cannot update caps");
    }
}

void AttachSource::addSource() const
{
    GstCaps *caps = createCapsFromMediaSource(m_gstWrapper, m_glibWrapper, m_attachedSource);
    if (!caps)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create caps from media source");
        return;
    }
    gchar *capsStr = m_gstWrapper->gstCapsToString(caps);
    GstElement *appSrc = nullptr;
    if (m_attachedSource->getType() == MediaSourceType::AUDIO)
    {
        RIALTO_SERVER_LOG_MIL("Adding Audio appsrc with caps %s", capsStr);
        appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "audsrc");
    }
    else if (m_attachedSource->getType() == MediaSourceType::VIDEO)
    {
        RIALTO_SERVER_LOG_MIL("Adding Video appsrc with caps %s", capsStr);
        appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "vidsrc");
    }
    else if (m_attachedSource->getType() == MediaSourceType::SUBTITLE)
    {
        RIALTO_SERVER_LOG_MIL("Adding Subtitle appsrc with caps %s", capsStr);
        appSrc = m_gstWrapper->gstElementFactoryMake("appsrc", "subsrc");

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(m_context.pipeline), "text-sink"))
        {
            GstElement *elem = m_gstTextTrackSinkFactory->createGstTextTrackSink();
            m_context.subtitleSink = elem;

            m_glibWrapper->gObjectSet(m_context.pipeline, "text-sink", elem, nullptr);
        }
    }
    m_glibWrapper->gFree(capsStr);

    m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(appSrc), caps);
    m_context.streamInfo.emplace(m_attachedSource->getType(), StreamInfo{appSrc, m_attachedSource->getHasDrm()});

    if (caps)
        m_gstWrapper->gstCapsUnref(caps);
}

void AttachSource::reattachAudioSource() const
{
    if (!m_player.reattachSource(m_attachedSource))
    {
        RIALTO_SERVER_LOG_ERROR("Reattaching source failed!");
        return;
    }

    m_context.streamInfo[m_attachedSource->getType()].isDataNeeded = true;
    m_context.audioSourceRemoved = false;
    m_player.notifyNeedMediaData(MediaSourceType::AUDIO);

    RIALTO_SERVER_LOG_MIL("Audio source reattached");
}

void AttachSource::reattachSubtitleSource() const
{
    // The subtitle appsrc and the text track sink are kept for the whole pipeline lifetime. When the client attaches
    // a subtitle source with a different format (e.g. CC -> TTML), update the appsrc caps. Appsrc sends the new caps
    // downstream with the next buffer and the text track sink switches the TextTrack session to the new data type.
    GstCaps *caps = createCapsFromMediaSource(m_gstWrapper, m_glibWrapper, m_attachedSource);
    if (!caps)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create caps from media source");
        return;
    }
    GstAppSrc *appSrc{GST_APP_SRC(m_context.streamInfo[MediaSourceType::SUBTITLE].appSrc)};
    GstCaps *oldCaps = m_gstWrapper->gstAppSrcGetCaps(appSrc);
    if (oldCaps && m_gstWrapper->gstCapsIsEqual(caps, oldCaps))
    {
        RIALTO_SERVER_LOG_MIL("Subtitle source reattached, caps unchanged");
    }
    else
    {
        gchar *capsStr = m_gstWrapper->gstCapsToString(caps);
        RIALTO_SERVER_LOG_MIL("Updating Subtitle appsrc caps to %s", capsStr);
        m_glibWrapper->gFree(capsStr);

        m_gstWrapper->gstAppSrcSetCaps(appSrc, caps);
    }

    if (oldCaps)
        m_gstWrapper->gstCapsUnref(oldCaps);
    m_gstWrapper->gstCapsUnref(caps);

    if (m_context.subtitleSourceRemoved)
    {
        m_context.subtitleSourceRemoved = false;
        // Data requests were blocked while the source was removed. Request data for the reattached source, unless
        // a request is already pending - only one request per source can be active, as each request clears the
        // shared memory partition of the source.
        StreamInfo &streamInfo = m_context.streamInfo[MediaSourceType::SUBTITLE];
        if (!streamInfo.isNeedDataPending)
        {
            streamInfo.isDataNeeded = true;
            m_player.notifyNeedMediaData(MediaSourceType::SUBTITLE);
        }
    }
}
} // namespace firebolt::rialto::server::tasks::generic
