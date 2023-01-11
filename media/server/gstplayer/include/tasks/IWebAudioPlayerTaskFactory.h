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

#ifndef FIREBOLT_RIALTO_SERVER_I_WEB_AUDIO_PLAYER_TASK_FACTORY_H_
#define FIREBOLT_RIALTO_SERVER_I_WEB_AUDIO_PLAYER_TASK_FACTORY_H_

#include "WebAudioPlayerContext.h"
#include "IGstWebAudioPlayerPrivate.h"
#include "IPlayerTask.h"

namespace firebolt::rialto::server
{
/**
 * @brief IWebAudioPlayerTaskFactory factory class, returns a concrete implementation of IPlayerTask
 */
class IWebAudioPlayerTaskFactory
{
public:
    IWebAudioPlayerTaskFactory() = default;
    virtual ~IWebAudioPlayerTaskFactory() = default;

    /**
     * @brief Creates a Shutdown task.
     *
     * @param[in] context       : The GstWebAudioPlayer context
     *
     * @retval the new Shutdown task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createShutdown(IGstWebAudioPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a Stop task.
     *
     * @param[in] player     : The GstWebAudioPlayer instance
     *
     * @retval the new Stop task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createStop(IGstWebAudioPlayerPrivate &player) const = 0;

};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_WEB_AUDIO_PLAYER_TASK_FACTORY_H_
