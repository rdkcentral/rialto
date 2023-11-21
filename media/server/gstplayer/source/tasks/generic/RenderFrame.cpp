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

#include "tasks/generic/RenderFrame.h"
#include "GenericPlayerContext.h"
#include "IGstGenericPlayerClient.h"
#include "IGstWrapper.h"
#include "RialtoServerLogging.h"
#include <gst/gst.h>

namespace firebolt::rialto::server::tasks::generic
{
RenderFrame::RenderFrame(GenericPlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper,
                         std::shared_ptr<IGlibWrapper> glibWrapper, IGstGenericPlayerPrivate &player)
    : m_context{context}, m_gstWrapper{gstWrapper}, m_glibWrapper(glibWrapper), m_player{player}
{
}

void RenderFrame::execute() const
{
    static const std::string kStepOnPrerollPropertyName = "frame-step-on-preroll";
    GstElement *videoSink = nullptr;

    m_glibWrapper->gObjectGet(m_context.pipeline, "video-sink", &videoSink, nullptr);
    if (videoSink)
    {
        // For AutoVideoSink we set properties on the child sink
        GstElement *actualVideoSink = m_player.getSinkChildIfAutoVideoSink(videoSink);
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(actualVideoSink),
                                                    kStepOnPrerollPropertyName.c_str()))
        {
            RIALTO_SERVER_LOG_INFO("Rendering preroll");

            m_glibWrapper->gObjectSet(actualVideoSink, kStepOnPrerollPropertyName.c_str(), 1, nullptr);
            m_gstWrapper->gstElementSendEvent(actualVideoSink,
                                              m_gstWrapper->gstEventNewStep(GST_FORMAT_BUFFERS, 1, 1.0, true, false));
            m_glibWrapper->gObjectSet(actualVideoSink, kStepOnPrerollPropertyName.c_str(), 0, nullptr);
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Video sink doesn't have property `%s`", kStepOnPrerollPropertyName.c_str());
        }

        m_gstWrapper->gstObjectUnref(GST_OBJECT(videoSink));
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("There's no video sink");
    }
}
} // namespace firebolt::rialto::server::tasks::generic
