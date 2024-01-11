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

#include "tasks/generic/RemoveSource.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::tasks::generic
{
RemoveSource::RemoveSource(GenericPlayerContext &context, IGstGenericPlayerClient *client,
                           std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper,
                           const MediaSourceType &type)
    : m_context{context}, m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}, m_type{type}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing RemoveSource");
}

RemoveSource::~RemoveSource()
{
    RIALTO_SERVER_LOG_DEBUG("RemoveSource finished");
}

void RemoveSource::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing RemoveSource");
    if (MediaSourceType::AUDIO != m_type)
    {
        RIALTO_SERVER_LOG_DEBUG("RemoveSource not supported for type != AUDIO");
        return;
    }
    m_context.audioBuffers.clear();
    m_context.audioNeedData = false;
    m_context.audioNeedDataPending = false;
    m_context.audioSourceRemoved = true;
    m_gstPlayerClient->invalidateActiveRequests(m_type);
    GstElement *source{nullptr};
    auto sourceElem = m_context.streamInfo.find(m_type);
    if (sourceElem != m_context.streamInfo.end())
    {
        source = sourceElem->second.appSrc;
    }
    if (!source)
    {
        RIALTO_SERVER_LOG_WARN("failed to flush - source is NULL");
        return;
    }
    GstEvent *flushStart = m_gstWrapper->gstEventNewFlushStart();
    if (!m_gstWrapper->gstElementSendEvent(source, flushStart))
    {
        RIALTO_SERVER_LOG_WARN("failed to send flush-start event");
    }
    GstEvent *flushStop = m_gstWrapper->gstEventNewFlushStop(FALSE);
    if (!m_gstWrapper->gstElementSendEvent(source, flushStop))
    {
        RIALTO_SERVER_LOG_WARN("failed to send flush-stop event");
    }

    // Turn audio off, removing audio sink from playsink
    // TODO: Move to common method
    GFlagsClass *flagsClass =
        static_cast<GFlagsClass *>(g_type_class_ref(g_type_from_name("GstPlayFlags")));
    GFlagsValue *flagVideo = g_flags_get_value_by_nick (flagsClass, "video");
    GFlagsValue *flagNativeVideo = g_flags_get_value_by_nick (flagsClass, "native-video");
    g_object_set(m_context.pipeline, "flags", flagVideo->value | flagNativeVideo->value , nullptr);
}
} // namespace firebolt::rialto::server::tasks::generic
