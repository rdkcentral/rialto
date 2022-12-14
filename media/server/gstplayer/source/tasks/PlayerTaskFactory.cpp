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
#include "tasks/PlayerTaskFactory.h"
#include "IMediaPipeline.h"
#include "tasks/AttachSamples.h"
#include "tasks/AttachSource.h"
#include "tasks/CheckAudioUnderflow.h"
#include "tasks/EnoughData.h"
#include "tasks/Eos.h"
#include "tasks/FinishSetupSource.h"
#include "tasks/HandleBusMessage.h"
#include "tasks/NeedData.h"
#include "tasks/Pause.h"
#include "tasks/Play.h"
#include "tasks/ReadShmDataAndAttachSamples.h"
#include "tasks/RenderFrame.h"
#include "tasks/ReportPosition.h"
#include "tasks/SetPlaybackRate.h"
#include "tasks/SetPosition.h"
#include "tasks/SetVideoGeometry.h"
#include "tasks/SetupElement.h"
#include "tasks/SetupSource.h"
#include "tasks/Shutdown.h"
#include "tasks/Stop.h"
#include "tasks/Underflow.h"

namespace firebolt::rialto::server
{
PlayerTaskFactory::PlayerTaskFactory(IGstPlayerClient *client, const std::shared_ptr<IGstWrapper> &gstWrapper,
                                     const std::shared_ptr<IGlibWrapper> &glibWrapper)
    : m_client{client}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}
{
}

std::unique_ptr<IPlayerTask>
PlayerTaskFactory::createAttachSamples(PlayerContext &context, IGstPlayerPrivate &player,
                                       const IMediaPipeline::MediaSegmentVector &mediaSegments) const
{
    return std::make_unique<AttachSamples>(context, player, mediaSegments);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createAttachSource(PlayerContext &context,
                                                                   std::unique_ptr<IMediaPipeline::MediaSource> &source) const
{
    return std::make_unique<AttachSource>(context, m_gstWrapper, m_glibWrapper, source);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createEnoughData(PlayerContext &context, GstAppSrc *src) const
{
    return std::make_unique<EnoughData>(context, src);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createEos(PlayerContext &context, IGstPlayerPrivate &player,
                                                          const firebolt::rialto::MediaSourceType &type) const
{
    return std::make_unique<Eos>(context, player, m_gstWrapper, type);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createFinishSetupSource(PlayerContext &context,
                                                                        IGstPlayerPrivate &player) const
{
    return std::make_unique<FinishSetupSource>(context, player, m_client);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createHandleBusMessage(PlayerContext &context, IGstPlayerPrivate &player,
                                                                       GstMessage *message) const
{
    return std::make_unique<HandleBusMessage>(context, player, m_client, m_gstWrapper, message);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createNeedData(PlayerContext &context, GstAppSrc *src) const
{
    return std::make_unique<NeedData>(context, m_client, src);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createPause(IGstPlayerPrivate &player) const
{
    return std::make_unique<Pause>(player);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createPlay(IGstPlayerPrivate &player) const
{
    return std::make_unique<Play>(player);
}

std::unique_ptr<IPlayerTask>
PlayerTaskFactory::createReadShmDataAndAttachSamples(PlayerContext &context, IGstPlayerPrivate &player,
                                                     const std::shared_ptr<IDataReader> &dataReader) const
{
    return std::make_unique<ReadShmDataAndAttachSamples>(context, player, dataReader);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createReportPosition(PlayerContext &context) const
{
    return std::make_unique<ReportPosition>(context, m_client, m_gstWrapper);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createCheckAudioUnderflow(PlayerContext &context,
                                                                          IGstPlayerPrivate &player) const
{
    return std::make_unique<CheckAudioUnderflow>(context, player, m_client, m_gstWrapper);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createSetPlaybackRate(PlayerContext &context, double rate) const
{
    return std::make_unique<SetPlaybackRate>(context, m_gstWrapper, m_glibWrapper, rate);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createSetPosition(PlayerContext &context, IGstPlayerPrivate &player,
                                                                  std::int64_t position) const
{
    return std::make_unique<SetPosition>(context, player, m_client, m_gstWrapper, position);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createSetupElement(PlayerContext &context, IGstPlayerPrivate &player,
                                                                   GstElement *element) const
{
    return std::make_unique<SetupElement>(context, m_gstWrapper, m_glibWrapper, player, element);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createSetupSource(PlayerContext &context, IGstPlayerPrivate &player,
                                                                  GstElement *source) const
{
    return std::make_unique<SetupSource>(context, player, source);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createSetVideoGeometry(PlayerContext &context, IGstPlayerPrivate &player,
                                                                       const Rectangle &rectangle) const
{
    return std::make_unique<SetVideoGeometry>(context, player, rectangle);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createShutdown(IGstPlayerPrivate &player) const
{
    return std::make_unique<Shutdown>(player);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createStop(PlayerContext &context, IGstPlayerPrivate &player) const
{
    return std::make_unique<Stop>(context, player);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createUnderflow(IGstPlayerPrivate &player, bool &underflowFlag) const
{
    return std::make_unique<Underflow>(player, m_client, underflowFlag);
}

std::unique_ptr<IPlayerTask> PlayerTaskFactory::createRenderFrame(PlayerContext &context) const
{
    return std::make_unique<RenderFrame>(context, m_gstWrapper, m_glibWrapper);
}

} // namespace firebolt::rialto::server
