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

#include "tasks/generic/HandleBusMessage.h"
#include "GenericPlayerContext.h"
#include "IGstGenericPlayerClient.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
HandleBusMessage::HandleBusMessage(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                   IGstGenericPlayerClient *client, std::shared_ptr<IGstWrapper> gstWrapper,
                                   GstMessage *message)
    : m_context{context}, m_player{player}, m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}, m_message{message}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing HandleBusMessage");
}

HandleBusMessage::~HandleBusMessage()
{
    RIALTO_SERVER_LOG_DEBUG("HandleBusMessage finished");
}

void HandleBusMessage::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing HandleBusMessage");
    switch (GST_MESSAGE_TYPE(m_message))
    {
    case GST_MESSAGE_STATE_CHANGED:
    {
        if (m_context.pipeline && GST_MESSAGE_SRC(m_message) == GST_OBJECT(m_context.pipeline))
        {
            GstState oldState, newState, pending;
            m_gstWrapper->gstMessageParseStateChanged(m_message, &oldState, &newState, &pending);
            RIALTO_SERVER_LOG_INFO("State changed (old: %s, new: %s, pending: %s)",
                                   m_gstWrapper->gstElementStateGetName(oldState),
                                   m_gstWrapper->gstElementStateGetName(newState),
                                   m_gstWrapper->gstElementStateGetName(pending));

            std::string filename = std::string(m_gstWrapper->gstElementStateGetName(oldState)) + "-" +
                                   std::string(m_gstWrapper->gstElementStateGetName(newState));
            m_gstWrapper->gstDebugBinToDotFileWithTs(GST_BIN(m_context.pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                                                     filename.c_str());
            if (!m_gstPlayerClient)
            {
                break;
            }
            switch (newState)
            {
            case GST_STATE_NULL:
            {
                m_gstPlayerClient->notifyPlaybackState(PlaybackState::STOPPED);
                break;
            }
            case GST_STATE_PAUSED:
            {
                if (pending != GST_STATE_PAUSED)
                {
                    // newState==GST_STATE_PAUSED, pending==GST_STATE_PAUSED state transition is received as a result of
                    // waiting for preroll after seek.
                    // Subsequent newState==GST_STATE_PAUSED, pending!=GST_STATE_PAUSED transition will
                    // indicate that the pipeline is prerolled and it reached GST_STATE_PAUSED state after seek.
                    m_gstPlayerClient->notifyPlaybackState(PlaybackState::PAUSED);
                }
                break;
            }
            case GST_STATE_PLAYING:
            {
                m_gstPlayerClient->notifyPlaybackState(PlaybackState::PLAYING);
                if (m_context.pendingPlaybackRate != kNoPendingPlaybackRate)
                {
                    m_player.setPendingPlaybackRate();
                }
                m_player.startPositionReportingAndCheckAudioUnderflowTimer();
                break;
            }
            case GST_STATE_VOID_PENDING:
            case GST_STATE_READY:
            {
                break;
            }
            }
        }
        break;
    }
    case GST_MESSAGE_EOS:
    {
        if (m_context.pipeline && GST_MESSAGE_SRC(m_message) == GST_OBJECT(m_context.pipeline))
        {
            RIALTO_SERVER_LOG_MIL("End of stream reached.");
            if (m_gstPlayerClient)
            {
                m_gstPlayerClient->notifyPlaybackState(PlaybackState::END_OF_STREAM);
            }
        }
        break;
    }
    case GST_MESSAGE_QOS:
    {
        GstFormat format;
        gboolean isLive = FALSE;
        guint64 runningTime = 0;
        guint64 streamTime = 0;
        guint64 timestamp = 0;
        guint64 duration = 0;
        guint64 dropped = 0;
        guint64 processed = 0;

        m_gstWrapper->gstMessageParseQos(m_message, &isLive, &runningTime, &streamTime, &timestamp, &duration);
        m_gstWrapper->gstMessageParseQosStats(m_message, &format, &processed, &dropped);

        if (GST_FORMAT_BUFFERS == format || GST_FORMAT_DEFAULT == format)
        {
            RIALTO_SERVER_LOG_INFO("QOS message: runningTime  %" G_GUINT64_FORMAT ", streamTime %" G_GUINT64_FORMAT
                                   ", timestamp %" G_GUINT64_FORMAT ", duration %" G_GUINT64_FORMAT
                                   ", format %u, processed %" G_GUINT64_FORMAT ", dropped %" G_GUINT64_FORMAT,
                                   runningTime, streamTime, timestamp, duration, format, processed, dropped);

            if (m_gstPlayerClient)
            {
                firebolt::rialto::QosInfo qosInfo = {processed, dropped};
                const gchar *klass;
                klass = m_gstWrapper->gstElementClassGetMetadata(GST_ELEMENT_GET_CLASS(GST_MESSAGE_SRC(m_message)),
                                                                 GST_ELEMENT_METADATA_KLASS);

                if (g_strrstr(klass, "Video"))
                {
                    m_gstPlayerClient->notifyQos(firebolt::rialto::MediaSourceType::VIDEO, qosInfo);
                }
                else if (g_strrstr(klass, "Audio"))
                {
                    m_gstPlayerClient->notifyQos(firebolt::rialto::MediaSourceType::AUDIO, qosInfo);
                }
                else
                {
                    RIALTO_SERVER_LOG_WARN("Unknown source type for class '%s', ignoring QOS Message", klass);
                }
            }
        }
        else
        {
            RIALTO_SERVER_LOG_WARN("Received a QOS_MESSAGE with unhandled format %s",
                                   m_gstWrapper->gstFormatGetName(format));
        }
        break;
    }
    default:
        break;
    }

    m_gstWrapper->gstMessageUnref(m_message);
}
} // namespace firebolt::rialto::server
