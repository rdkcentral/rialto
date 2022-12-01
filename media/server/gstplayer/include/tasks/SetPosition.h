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

#ifndef FIREBOLT_RIALTO_SERVER_SET_POSITION_H_
#define FIREBOLT_RIALTO_SERVER_SET_POSITION_H_

#include "IGstPlayerClient.h"
#include "IGstPlayerPrivate.h"
#include "IGstWrapper.h"
#include "IPlayerTask.h"
#include "PlayerContext.h"
#include <cstdint>
#include <memory>

namespace firebolt::rialto::server
{
class SetPosition : public IPlayerTask
{
public:
    SetPosition(PlayerContext &context, IGstPlayerPrivate &player, IGstPlayerClient *client,
                std::shared_ptr<IGstWrapper> gstWrapper, std::int64_t position);
    ~SetPosition() override;
    void execute() const override;

private:
    PlayerContext &m_context;
    IGstPlayerClient *m_gstPlayerClient;
    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::int64_t m_position;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_SET_POSITION_H_
