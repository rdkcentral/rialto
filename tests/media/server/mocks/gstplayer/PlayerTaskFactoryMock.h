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

#ifndef FIREBOLT_RIALTO_SERVER_PLAYER_TASK_FACTORY_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_PLAYER_TASK_FACTORY_MOCK_H_
#include "tasks/IPlayerTaskFactory.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::server
{
class PlayerTaskFactoryMock : public IPlayerTaskFactory
{
public:
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createAttachSamples,
                (PlayerContext & context, IGstPlayerPrivate &player,
                 const IMediaPipeline::MediaSegmentVector &mediaSegments),
                (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createAttachSource,
                (PlayerContext & context, std::unique_ptr<IMediaPipeline::MediaSource> &source), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createEnoughData, (PlayerContext & context, GstAppSrc *src),
                (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createEos,
                (PlayerContext & context, IGstPlayerPrivate &player, const firebolt::rialto::MediaSourceType &type),
                (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createFinishSetupSource,
                (PlayerContext & context, IGstPlayerPrivate &player), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createHandleBusMessage,
                (PlayerContext & context, IGstPlayerPrivate &player, GstMessage *message), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createNeedData, (PlayerContext & context, GstAppSrc *src),
                (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createPause, (IGstPlayerPrivate & player), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createPlay, (IGstPlayerPrivate & player), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createReadShmDataAndAttachSamples,
                (PlayerContext & context, IGstPlayerPrivate &player, const std::shared_ptr<IDataReader> &dataReader),
                (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createReportPosition, (PlayerContext & context), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createCheckAudioUnderflow,
                (PlayerContext & context, IGstPlayerPrivate &player), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createSetPlaybackRate, (PlayerContext & context, double rate),
                (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createSetPosition,
                (PlayerContext & context, IGstPlayerPrivate &player, std::int64_t position), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createSetupElement,
                (PlayerContext & context, IGstPlayerPrivate &player, GstElement *element), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createSetupSource,
                (PlayerContext & context, IGstPlayerPrivate &player, GstElement *source), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createSetVideoGeometry,
                (PlayerContext & context, IGstPlayerPrivate &player, const Rectangle &rectangle), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createShutdown, (IGstPlayerPrivate & player), (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createStop, (PlayerContext & context, IGstPlayerPrivate &player),
                (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createUnderflow, (IGstPlayerPrivate & player, bool &underflowFlag),
                (const, override));
    MOCK_METHOD(std::unique_ptr<IPlayerTask>, createRenderFrame, (PlayerContext & context), (const, override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_PLAYER_TASK_FACTORY_MOCK_H_
