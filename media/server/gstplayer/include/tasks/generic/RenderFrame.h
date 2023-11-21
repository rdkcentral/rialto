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

#ifndef FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_RENDER_FRAME_H_
#define FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_RENDER_FRAME_H_

#include "GenericPlayerContext.h"
#include "IGlibWrapper.h"
#include "IGstGenericPlayerPrivate.h"
#include "IGstWrapper.h"
#include "IPlayerTask.h"
#include <cstdint>
#include <memory>

namespace firebolt::rialto::server::tasks::generic
{
class RenderFrame : public IPlayerTask
{
public:
    RenderFrame(GenericPlayerContext &context, std::shared_ptr<IGstWrapper> gstWrapper,
                std::shared_ptr<IGlibWrapper> glibWrapper, IGstGenericPlayerPrivate &player);
    ~RenderFrame() override = default;
    void execute() const override;

private:
    GenericPlayerContext &m_context;
    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::shared_ptr<IGlibWrapper> m_glibWrapper;
    IGstGenericPlayerPrivate &m_player;
};
} // namespace firebolt::rialto::server::tasks::generic

#endif // FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_RENDER_FRAME_H_
