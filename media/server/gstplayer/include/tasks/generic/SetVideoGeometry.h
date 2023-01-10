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

#ifndef FIREBOLT_RIALTO_SERVER_SET_VIDEO_GEOMETRY_H_
#define FIREBOLT_RIALTO_SERVER_SET_VIDEO_GEOMETRY_H_

#include "GenericPlayerContext.h"
#include "IGstGenericPlayerPrivate.h"
#include "IPlayerTask.h"

namespace firebolt::rialto::server
{
class SetVideoGeometry : public IPlayerTask
{
public:
    SetVideoGeometry(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, const Rectangle &rectangle);
    ~SetVideoGeometry() override;
    void execute() const override;

private:
    GenericPlayerContext &m_context;
    IGstGenericPlayerPrivate &m_player;
    Rectangle m_rectangle;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_SET_VIDEO_GEOMETRY_H_
