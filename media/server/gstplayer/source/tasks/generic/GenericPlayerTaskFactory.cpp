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
#include "tasks/generic/GenericPlayerTaskFactory.h"
#include "IMediaPipeline.h"
#include "tasks/generic/AttachSamples.h"
#include "tasks/generic/AttachSource.h"
#include "tasks/generic/CheckAudioUnderflow.h"
#include "tasks/generic/DeepElementAdded.h"
#include "tasks/generic/EnoughData.h"
#include "tasks/generic/Eos.h"
#include "tasks/generic/FinishSetupSource.h"
#include "tasks/generic/Flush.h"
#include "tasks/generic/HandleBusMessage.h"
#include "tasks/generic/NeedData.h"
#include "tasks/generic/Pause.h"
#include "tasks/generic/Ping.h"
#include "tasks/generic/Play.h"
#include "tasks/generic/ReadShmDataAndAttachSamples.h"
#include "tasks/generic/RemoveSource.h"
#include "tasks/generic/RenderFrame.h"
#include "tasks/generic/ReportPosition.h"
#include "tasks/generic/SetMute.h"
#include "tasks/generic/SetPlaybackRate.h"
#include "tasks/generic/SetPosition.h"
#include "tasks/generic/SetVideoGeometry.h"
#include "tasks/generic/SetVolume.h"
#include "tasks/generic/SetupElement.h"
#include "tasks/generic/SetupSource.h"
#include "tasks/generic/Shutdown.h"
#include "tasks/generic/Stop.h"
#include "tasks/generic/Underflow.h"
#include "tasks/generic/UpdatePlaybackGroup.h"

namespace firebolt::rialto::server
{
GenericPlayerTaskFactory::GenericPlayerTaskFactory(
    IGstGenericPlayerClient *client, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> &rdkGstreamerUtilsWrapper)
    : m_client{client}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}, m_rdkGstreamerUtilsWrapper{
                                                                                  rdkGstreamerUtilsWrapper}
{
}

std::unique_ptr<IPlayerTask>
GenericPlayerTaskFactory::createAttachSamples(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                              const IMediaPipeline::MediaSegmentVector &mediaSegments) const
{
    return std::make_unique<tasks::generic::AttachSamples>(context, player, mediaSegments);
}

std::unique_ptr<IPlayerTask>
GenericPlayerTaskFactory::createAttachSource(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                             const std::unique_ptr<IMediaPipeline::MediaSource> &source) const
{
    return std::make_unique<tasks::generic::AttachSource>(context, m_gstWrapper, m_glibWrapper,
                                                          m_rdkGstreamerUtilsWrapper, player, source);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createDeepElementAdded(GenericPlayerContext &context,
                                                                              IGstGenericPlayerPrivate &player,
                                                                              GstBin *pipeline, GstBin *bin,
                                                                              GstElement *element) const
{
    return std::make_unique<tasks::generic::DeepElementAdded>(context, player, m_gstWrapper, m_glibWrapper, pipeline,
                                                              bin, element);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createEnoughData(GenericPlayerContext &context, GstAppSrc *src) const
{
    return std::make_unique<tasks::generic::EnoughData>(context, src);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createEos(GenericPlayerContext &context,
                                                                 IGstGenericPlayerPrivate &player,
                                                                 const firebolt::rialto::MediaSourceType &type) const
{
    return std::make_unique<tasks::generic::Eos>(context, player, m_gstWrapper, type);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createFinishSetupSource(GenericPlayerContext &context,
                                                                               IGstGenericPlayerPrivate &player) const
{
    return std::make_unique<tasks::generic::FinishSetupSource>(context, player, m_client);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createHandleBusMessage(GenericPlayerContext &context,
                                                                              IGstGenericPlayerPrivate &player,
                                                                              GstMessage *message) const
{
    return std::make_unique<tasks::generic::HandleBusMessage>(context, player, m_client, m_gstWrapper, m_glibWrapper,
                                                              message);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createNeedData(GenericPlayerContext &context, GstAppSrc *src) const
{
    return std::make_unique<tasks::generic::NeedData>(context, m_client, src);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createPause(GenericPlayerContext &context,
                                                                   IGstGenericPlayerPrivate &player) const
{
    return std::make_unique<tasks::generic::Pause>(context, player);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createPlay(IGstGenericPlayerPrivate &player) const
{
    return std::make_unique<tasks::generic::Play>(player);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createReadShmDataAndAttachSamples(
    GenericPlayerContext &context, IGstGenericPlayerPrivate &player, const std::shared_ptr<IDataReader> &dataReader) const
{
    return std::make_unique<tasks::generic::ReadShmDataAndAttachSamples>(context, player, dataReader);
}

std::unique_ptr<IPlayerTask>
GenericPlayerTaskFactory::createRemoveSource(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                             const firebolt::rialto::MediaSourceType &type) const
{
    return std::make_unique<tasks::generic::RemoveSource>(context, player, m_client, m_gstWrapper, type);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createReportPosition(GenericPlayerContext &context) const
{
    return std::make_unique<tasks::generic::ReportPosition>(context, m_client, m_gstWrapper);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createCheckAudioUnderflow(GenericPlayerContext &context,
                                                                                 IGstGenericPlayerPrivate &player) const
{
    return std::make_unique<tasks::generic::CheckAudioUnderflow>(context, player, m_client, m_gstWrapper);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createSetPlaybackRate(GenericPlayerContext &context,
                                                                             double rate) const
{
    return std::make_unique<tasks::generic::SetPlaybackRate>(context, m_gstWrapper, m_glibWrapper, rate);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createSetPosition(GenericPlayerContext &context,
                                                                         IGstGenericPlayerPrivate &player,
                                                                         std::int64_t position) const
{
    return std::make_unique<tasks::generic::SetPosition>(context, player, m_client, m_gstWrapper, position);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createSetupElement(GenericPlayerContext &context,
                                                                          IGstGenericPlayerPrivate &player,
                                                                          GstElement *element) const
{
    return std::make_unique<tasks::generic::SetupElement>(context, m_gstWrapper, m_glibWrapper, player, element);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createSetupSource(GenericPlayerContext &context,
                                                                         IGstGenericPlayerPrivate &player,
                                                                         GstElement *source) const
{
    return std::make_unique<tasks::generic::SetupSource>(context, player, source);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createSetVideoGeometry(GenericPlayerContext &context,
                                                                              IGstGenericPlayerPrivate &player,
                                                                              const Rectangle &rectangle) const
{
    return std::make_unique<tasks::generic::SetVideoGeometry>(context, player, rectangle);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createSetVolume(GenericPlayerContext &context, double volume) const
{
    return std::make_unique<tasks::generic::SetVolume>(context, m_gstWrapper, volume);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createSetMute(GenericPlayerContext &context, bool mute) const
{
    return std::make_unique<tasks::generic::SetMute>(context, m_gstWrapper, mute);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createShutdown(IGstGenericPlayerPrivate &player) const
{
    return std::make_unique<tasks::generic::Shutdown>(player);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createStop(GenericPlayerContext &context,
                                                                  IGstGenericPlayerPrivate &player) const
{
    return std::make_unique<tasks::generic::Stop>(context, player);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createUnderflow(GenericPlayerContext &context,
                                                                       IGstGenericPlayerPrivate &player,
                                                                       bool &underflowFlag, bool underflowEnabled,
                                                                       MediaSourceType sourceType) const
{
    return std::make_unique<tasks::generic::Underflow>(context, player, m_client, underflowFlag, underflowEnabled,
                                                       sourceType);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createUpdatePlaybackGroup(GenericPlayerContext &context,
                                                                                 GstElement *typefind,
                                                                                 const GstCaps *caps) const
{
    return std::make_unique<tasks::generic::UpdatePlaybackGroup>(context, m_gstWrapper, m_glibWrapper, typefind, caps);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createRenderFrame(GenericPlayerContext &context,
                                                                         IGstGenericPlayerPrivate &player) const
{
    return std::make_unique<tasks::generic::RenderFrame>(context, m_gstWrapper, m_glibWrapper, player);
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createPing(std::unique_ptr<IHeartbeatHandler> &&heartbeatHandler) const
{
    return std::make_unique<tasks::generic::Ping>(std::move(heartbeatHandler));
}

std::unique_ptr<IPlayerTask> GenericPlayerTaskFactory::createFlush(GenericPlayerContext &context,
                                                                   IGstGenericPlayerPrivate &player,
                                                                   const firebolt::rialto::MediaSourceType &type,
                                                                   bool resetTime) const
{
    return std::make_unique<tasks::generic::Flush>(context, player, m_client, m_gstWrapper, type, resetTime);
}
} // namespace firebolt::rialto::server
