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

#ifndef FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_SET_USE_BUFFERING_H_
#define FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_SET_USE_BUFFERING_H_

#include "GenericPlayerContext.h"
#include "IGstGenericPlayerPrivate.h"
#include "IPlayerTask.h"
#include <memory>

namespace firebolt::rialto::server::tasks::generic
{
class SetUseBuffering : public IPlayerTask
{
public:
    SetUseBuffering(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, bool useBuffering);
    ~SetUseBuffering() override;
    void execute() const override;

private:
    GenericPlayerContext &m_context;
    IGstGenericPlayerPrivate &m_player;
    bool m_useBuffering;
};
} // namespace firebolt::rialto::server::tasks::generic

#endif // FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_SET_USE_BUFFERING_H_