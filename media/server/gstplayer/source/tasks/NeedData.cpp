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

#include "tasks/NeedData.h"
#include "IGstPlayerClient.h"
#include "PlayerContext.h"
#include "RialtoServerLogging.h"
#include <gst/gst.h>

namespace firebolt::rialto::server
{
NeedData::NeedData(PlayerContext &context, IGstPlayerClient *client, GstAppSrc *src)
    : m_context{context}, m_gstPlayerClient{client}, m_src{src}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing NeedData");
}

NeedData::~NeedData()
{
    RIALTO_SERVER_LOG_DEBUG("NeedData finished");
}

void NeedData::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing NeedData");
    auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO);
    if (elem != m_context.streamInfo.end())
    {
        if (elem->second == GST_ELEMENT(m_src))
        {
            m_context.audioNeedData = true;
            if (m_gstPlayerClient && !m_context.audioNeedDataPending)
            {
                m_context.audioNeedDataPending = m_gstPlayerClient->notifyNeedMediaData(MediaSourceType::AUDIO);
            }
        }
    }
    elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO);
    if (elem != m_context.streamInfo.end())
    {
        if (elem->second == GST_ELEMENT(m_src))
        {
            m_context.videoNeedData = true;
            if (m_gstPlayerClient && !m_context.videoNeedDataPending)
            {
                m_context.videoNeedDataPending = m_gstPlayerClient->notifyNeedMediaData(MediaSourceType::VIDEO);
            }
        }
    }
}
} // namespace firebolt::rialto::server
