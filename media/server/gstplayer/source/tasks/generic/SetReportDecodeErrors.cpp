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

#include "SetReportDecodeErrors.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"

namespace firebolt::rialto::server::tasks::generic
{
SetReportDecodeErrors::SetReportDecodeErrors(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                       const MediaSourceType &type, bool reportDecodeErrors)
    : m_context{context}, m_player(player), m_type{type}, m_reportDecodeErrors{reportDecodeErrors}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing SetReportDecodeErrors");
}

SetReportDecodeErrors::~SetReportDecodeErrors()
{
    RIALTO_SERVER_LOG_DEBUG("SetReportDecodeErrors finished");
}

void SetReportDecodeErrors::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing SetReportDecodeErrors for %s source", common::convertMediaSourceType(m_type));

    if (m_type != MediaSourceType::VIDEO)
    {
        RIALTO_SERVER_LOG_ERROR("SetReportDecodeErrors not currently supported for non-video");
    }

    if (m_context.pipeline)
    {
        m_player.setReportDecodeErrors(m_reportDecodeErrors);
    }
}
} // namespace firebolt::rialto::server::tasks::generic
