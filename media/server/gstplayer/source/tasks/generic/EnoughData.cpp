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

#include "tasks/generic/EnoughData.h"
#include "GenericPlayerContext.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
EnoughData::EnoughData(GenericPlayerContext &context, GstAppSrc *src) : m_context{context}, m_src{src}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing EnoughData");
}

EnoughData::~EnoughData()
{
    RIALTO_SERVER_LOG_DEBUG("EnoughData finished");
}

void EnoughData::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing EnoughData");
    auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO);
    if (elem != m_context.streamInfo.end())
    {
        if (elem->second == GST_ELEMENT(m_src))
        {
            m_context.audioNeedData = false;
        }
    }
    elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO);
    if (elem != m_context.streamInfo.end())
    {
        if (elem->second == GST_ELEMENT(m_src))
        {
            m_context.videoNeedData = false;
        }
    }
}
} // namespace firebolt::rialto::server
