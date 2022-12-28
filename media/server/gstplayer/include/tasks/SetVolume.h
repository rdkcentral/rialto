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

#ifndef FIREBOLT_RIALTO_SERVER_SET_VOLUME_H_
#define FIREBOLT_RIALTO_SERVER_SET_VOLUME_H_

#include "IPlayerTask.h"
#include "PlayerContext.h"

namespace firebolt::rialto::server
{
class SetVolume : public IPlayerTask
{
public:
    SetVolume(PlayerContext &context, double volume);
    ~SetVolume() override;
    void execute() const override;

private:
    PlayerContext &m_context;
    double m_volume;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_SET_VOLUME_H_
