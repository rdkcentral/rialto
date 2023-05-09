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

#ifndef FIREBOLT_RIALTO_SERVER_GENERIC_PLAYER_TASK_FACTORY_H_
#define FIREBOLT_RIALTO_SERVER_GENERIC_PLAYER_TASK_FACTORY_H_

#include "IGenericPlayerTaskFactory.h"
#include "IGlibWrapper.h"
#include "IGstGenericPlayerClient.h"
#include "IGstWrapper.h"
#include "IMediaPipeline.h"
#include <memory>

namespace firebolt::rialto::server
{
class GenericPlayerTaskFactory : public IGenericPlayerTaskFactory
{
public:
    GenericPlayerTaskFactory(IGstGenericPlayerClient *client, const std::shared_ptr<IGstWrapper> &gstWrapper,
                             const std::shared_ptr<IGlibWrapper> &glibWrapper,
                             const std::shared_ptr<IRdkGstreamerUtilsWrapper> &rdkGstreamerUtilsWrapper);
    ~GenericPlayerTaskFactory() override = default;

    std::unique_ptr<IPlayerTask> createAttachSamples(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                     const IMediaPipeline::MediaSegmentVector &mediaSegments) const override;
    std::unique_ptr<IPlayerTask>
    createAttachSource(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                       const std::unique_ptr<IMediaPipeline::MediaSource> &source) const override;
    std::unique_ptr<IPlayerTask> createDeepElementAdded(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                        GstBin *pipeline, GstBin *bin,
                                                        GstElement *element) const override;
    std::unique_ptr<IPlayerTask> createEnoughData(GenericPlayerContext &context, GstAppSrc *src) const override;
    std::unique_ptr<IPlayerTask> createEos(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                           const firebolt::rialto::MediaSourceType &type) const override;
    std::unique_ptr<IPlayerTask> createFinishSetupSource(GenericPlayerContext &context,
                                                         IGstGenericPlayerPrivate &player) const override;
    std::unique_ptr<IPlayerTask> createHandleBusMessage(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                        GstMessage *message) const override;
    std::unique_ptr<IPlayerTask> createNeedData(GenericPlayerContext &context, GstAppSrc *src) const override;
    std::unique_ptr<IPlayerTask> createPause(GenericPlayerContext &context,
                                             IGstGenericPlayerPrivate &player) const override;
    std::unique_ptr<IPlayerTask> createPlay(IGstGenericPlayerPrivate &player) const override;
    std::unique_ptr<IPlayerTask>
    createReadShmDataAndAttachSamples(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                      const std::shared_ptr<IDataReader> &dataReader) const override;
    std::unique_ptr<IPlayerTask> createRemoveSource(GenericPlayerContext &context,
                                                    const firebolt::rialto::MediaSourceType &type) const override;
    std::unique_ptr<IPlayerTask> createReportPosition(GenericPlayerContext &context) const override;
    std::unique_ptr<IPlayerTask> createCheckAudioUnderflow(GenericPlayerContext &context,
                                                           IGstGenericPlayerPrivate &player) const override;
    std::unique_ptr<IPlayerTask> createSetPlaybackRate(GenericPlayerContext &context, double rate) const override;
    std::unique_ptr<IPlayerTask> createSetPosition(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                   std::int64_t position) const override;
    std::unique_ptr<IPlayerTask> createSetupElement(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                    GstElement *element) const override;
    std::unique_ptr<IPlayerTask> createSetupSource(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                   GstElement *source) const override;
    std::unique_ptr<IPlayerTask> createSetVideoGeometry(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                        const Rectangle &rectangle) const override;
    std::unique_ptr<IPlayerTask> createSetVolume(GenericPlayerContext &context, double volume) const override;
    std::unique_ptr<IPlayerTask> createShutdown(IGstGenericPlayerPrivate &player) const override;
    std::unique_ptr<IPlayerTask> createStop(GenericPlayerContext &context,
                                            IGstGenericPlayerPrivate &player) const override;
    std::unique_ptr<IPlayerTask> createUnderflow(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                 bool &underflowFlag, bool underflowEnable) const override;
    std::unique_ptr<IPlayerTask> createUpdatePlaybackGroup(GenericPlayerContext &context, GstElement *typefind,
                                                           const GstCaps *caps) const override;
    std::unique_ptr<IPlayerTask> createRenderFrame(GenericPlayerContext &context) const override;

private:
    IGstGenericPlayerClient *m_client;
    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::shared_ptr<IGlibWrapper> m_glibWrapper;
    std::shared_ptr<IRdkGstreamerUtilsWrapper> m_rdkGstreamerUtilsWrapper;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GENERIC_PLAYER_TASK_FACTORY_H_
