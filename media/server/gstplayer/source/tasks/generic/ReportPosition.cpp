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

#include "tasks/generic/ReportPosition.h"
#include "IGstGenericPlayerClient.h"
#include "IGstWrapper.h"
#include "GenericPlayerContext.h"
#include <gst/gst.h>

namespace firebolt::rialto::server
{
ReportPosition::ReportPosition(GenericPlayerContext &context, IGstGenericPlayerClient *client, std::shared_ptr<IGstWrapper> gstWrapper)
    : m_context{context}, m_gstPlayerClient{client}, m_gstWrapper{gstWrapper}
{
}

void ReportPosition::execute() const
{
    gint64 position = -1;
    m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &position);
    if (position >= 0)
    {
        if (m_gstPlayerClient)
        {
            m_gstPlayerClient->notifyPosition(position);
        }
    }
}
} // namespace firebolt::rialto::server
