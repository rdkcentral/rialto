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

#include "tasks/generic/DeepElementAdded.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
DeepElementAdded::DeepElementAdded(GenericPlayerContext &context,
                                   const std::shared_ptr<IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper,
                                   GstBin *pipeline, GstBin *bin, GstElement *element)
    : m_context{context}, m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper},
      m_pipeline{pipeline}, m_bin{bin}, m_element{element}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing DeepElementAdded");
}

DeepElementAdded::~DeepElementAdded()
{
    RIALTO_SERVER_LOG_DEBUG("DeepElementAdded finished");
}

void DeepElementAdded::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing DeepElementAdded");
    m_rdkGstreamerUtilsWrapper->deepElementAdded(&m_context.playbackGroup, m_pipeline, m_bin, m_element);
}
} // namespace firebolt::rialto::server
