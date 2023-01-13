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

#ifndef FIREBOLT_RIALTO_SERVER_DEEP_ELEMENT_ADDED_H_
#define FIREBOLT_RIALTO_SERVER_DEEP_ELEMENT_ADDED_H_

#include "GenericPlayerContext.h"
#include "IPlayerTask.h"
#include "IRdkGstreamerUtilsWrapper.h"
#include <gst/gst.h>
#include <memory>

namespace firebolt::rialto::server
{
class DeepElementAdded : public IPlayerTask
{
public:
    DeepElementAdded(GenericPlayerContext &context,
                     const std::shared_ptr<IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper, GstBin *pipeline,
                     GstBin *bin, GstElement *element);
    ~DeepElementAdded() override;
    void execute() const override;

private:
    GenericPlayerContext &m_context;
    std::shared_ptr<IRdkGstreamerUtilsWrapper> m_rdkGstreamerUtilsWrapper;
    GstBin *m_pipeline;
    GstBin *m_bin;
    GstElement *m_element;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_DEEP_ELEMENT_ADDED_H_
