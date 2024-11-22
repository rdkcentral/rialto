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

#ifndef FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_SWITCH_SOURCE_H_
#define FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_SWITCH_SOURCE_H_

#include "GenericPlayerContext.h"
#include "IGlibWrapper.h"
#include "IGstWrapper.h"
#include "IMediaPipeline.h"
#include "IPlayerTask.h"
#include "IRdkGstreamerUtilsWrapper.h"
#include <cstdint>
#include <memory>
#include <optional>

namespace firebolt::rialto::server::tasks::generic
{
class SwitchSource : public IPlayerTask
{
public:
    SwitchSource(GenericPlayerContext &context,
                 const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                 const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                 const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper,
                 const std::unique_ptr<IMediaPipeline::MediaSource> &source);
    ~SwitchSource() override;
    void execute() const override;

private:
    std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> createAudioAttributes() const;

private:
    GenericPlayerContext &m_context;
    std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> m_gstWrapper;
    std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> m_glibWrapper;
    std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> m_rdkGstreamerUtilsWrapper;
    std::unique_ptr<IMediaPipeline::MediaSource> m_source;
};
} // namespace firebolt::rialto::server::tasks::generic

#endif // FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_SWITCH_SOURCE_H_
