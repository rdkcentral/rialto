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

#include <stdexcept>

#include "RialtoServerLogging.h"
#include "SinkPtr.h"
#include "SinkPtrPrivate.h"

namespace firebolt::rialto::server
{
SinkPtrFactory::SinkPtrFactory(const GenericPlayerContext &context,
                               const firebolt::rialto::MediaSourceType &mediaSourceType,
                               const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                               const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper)
    : m_context(context), m_mediaSourceType(mediaSourceType), m_glibWrapper(glibWrapper), m_gstWrapper(gstWrapper)
{
}

std::shared_ptr<ISinkPtr> SinkPtrFactory::getSinkPtr() const
{
    GstElement *sink = getSink();
    if (sink)
    {
        // For AutoVideoSink or AutoAudioSink we use properties on the child sink
        GstElement *autoSink{sink};
        if (firebolt::rialto::MediaSourceType::VIDEO == m_mediaSourceType)
            autoSink = getSinkChildIfAutoVideoSink(sink);
        else if (firebolt::rialto::MediaSourceType::AUDIO == m_mediaSourceType)
            autoSink = getSinkChildIfAutoAudioSink(sink);

        if (autoSink != sink)
            return std::make_shared<SinkPtrToAuto>(autoSink, sink, m_gstWrapper);

        return std::make_shared<SinkPtrDirect>(sink, m_gstWrapper);
    }
    return std::make_shared<SinkPtrNull>();
}

GstElement *SinkPtrFactory::getSink() const
{
    const char *kSinkName{nullptr};
    GstElement *sink{nullptr};
    switch (m_mediaSourceType)
    {
    case MediaSourceType::AUDIO:
        kSinkName = "audio-sink";
        break;
    case MediaSourceType::VIDEO:
        kSinkName = "video-sink";
        break;
    default:
        break;
    }
    if (!kSinkName)
    {
        RIALTO_SERVER_LOG_WARN("mediaSourceType not supported %d", static_cast<int>(m_mediaSourceType));
    }
    else
    {
        m_glibWrapper->gObjectGet(m_context.pipeline, kSinkName, &sink, nullptr);
        if (!sink)
        {
            RIALTO_SERVER_LOG_WARN("%s could not be obtained", kSinkName);
        }
    }
    return sink;
}

GstElement *SinkPtrFactory::getSinkChildIfAutoVideoSink(GstElement *sink) const
{
    GstElement *result{nullptr};
    const gchar *kTmpName = m_glibWrapper->gTypeName(G_OBJECT_TYPE(sink));
    if (kTmpName)
    {
        const std::string kElementTypeName{kTmpName};
        if (kElementTypeName == "GstAutoVideoSink")
        {
            if (!m_context.autoVideoChildSink)
            {
                RIALTO_SERVER_LOG_WARN("No child sink has been added to the autovideosink");
            }
            else
            {
                result = m_context.autoVideoChildSink;
            }
        }
    }
    return result;
}

GstElement *SinkPtrFactory::getSinkChildIfAutoAudioSink(GstElement *sink) const
{
    GstElement *result{nullptr};
    const gchar *kTmpName = m_glibWrapper->gTypeName(G_OBJECT_TYPE(sink));
    if (kTmpName)
    {
        const std::string kElementTypeName{kTmpName};
        if (kElementTypeName == "GstAutoAudioSink")
        {
            if (!m_context.autoAudioChildSink)
            {
                RIALTO_SERVER_LOG_WARN("No child sink has been added to the autoaudiosink");
            }
            else
            {
                result = m_context.autoAudioChildSink;
            }
        }
    }
    return result;
}

SinkPtrDirect::SinkPtrDirect(GstElement *sink, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper)
    : m_sink(sink), m_gstWrapper(gstWrapper)
{
}

SinkPtrDirect::~SinkPtrDirect()
{
    m_gstWrapper->gstObjectUnref(GST_OBJECT(m_sink));
}

GstElement *SinkPtrDirect::getSink() const
{
    return m_sink;
}

SinkPtrToAuto::SinkPtrToAuto(GstElement *autoSink, GstElement *sinkToUnref,
                             const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper)
    : m_autoSink(autoSink), m_sinkToUnref(sinkToUnref), m_gstWrapper(gstWrapper)
{
}

SinkPtrToAuto::~SinkPtrToAuto()
{
    // We only need to unref the object returned by getSink()
    // m_autoSink should NOT be unreffed
    m_gstWrapper->gstObjectUnref(GST_OBJECT(m_sinkToUnref));
}

GstElement *SinkPtrToAuto::getSink() const
{
    return m_autoSink;
}

GstElement *SinkPtrNull::getSink() const
{
    return nullptr;
}

}; // namespace firebolt::rialto::server
