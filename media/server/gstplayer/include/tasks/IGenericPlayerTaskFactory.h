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

#ifndef FIREBOLT_RIALTO_SERVER_I_GENERIC_PLAYER_TASK_FACTORY_H_
#define FIREBOLT_RIALTO_SERVER_I_GENERIC_PLAYER_TASK_FACTORY_H_

#include "GenericPlayerContext.h"
#include "IDataReader.h"
#include "IGstGenericPlayerPrivate.h"
#include "IHeartbeatHandler.h"
#include "IMediaPipeline.h"
#include "IPlayerTask.h"
#include "MediaCommon.h"
#include <cstdint>
#include <gst/app/gstappsrc.h>
#include <memory>

namespace firebolt::rialto::server
{
/**
 * @brief IGenericPlayerTaskFactory factory class, returns a concrete implementation of IPlayerTask
 */
class IGenericPlayerTaskFactory
{
public:
    IGenericPlayerTaskFactory() = default;
    virtual ~IGenericPlayerTaskFactory() = default;

    /**
     * @brief Creates a AttachSamples task.
     *
     * @param[in] context       : The GstGenericPlayer context
     * @param[in] player        : The GstGenericPlayer instance
     * @param[in] mediaSegments : The media segments to attach
     *
     * @retval the new AttachSamples task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createAttachSamples(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                        const IMediaPipeline::MediaSegmentVector &mediaSegments) const = 0;

    /**
     * @brief Creates a AttachSource task.
     *
     * @param[in] context   : The GstGenericPlayer context
     * @param[in] player    : The GstGenericPlayer instance
     * @param[in] source    : The source to attach.
     *
     * @retval the new AttachSource task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createAttachSource(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                       const std::unique_ptr<IMediaPipeline::MediaSource> &source) const = 0;

    /**
     * @brief Creates a DeepElementAdded task.
     *
     * @param[in] context  : The GstPlayer context
     * @param[in] player        : The GstGenericPlayer instance
     * @param[in] pipeline : The pipeline the signal was fired from.
     * @param[in] bin      : the GstBin the element was added to
     * @param[in] element  : an element that was added to the playbin hierarchy
     *
     * @retval the new DeepElementAdded task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createDeepElementAdded(GenericPlayerContext &context,
                                                                IGstGenericPlayerPrivate &player, GstBin *pipeline,
                                                                GstBin *bin, GstElement *element) const = 0;

    /**
     * @brief Creates a EnoughData task.
     *
     * @param[in] context : The GstGenericPlayer context
     * @param[in] src     : The source, which reports enough data.
     *
     * @retval the new EnoughData task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createEnoughData(GenericPlayerContext &context, GstAppSrc *src) const = 0;

    /**
     * @brief Creates a Eos task.
     *
     * @param[in] context : The GstGenericPlayer context
     * @param[in] type    : The media source type, which reports eos.
     *
     * @retval the new Eos task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createEos(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                   const firebolt::rialto::MediaSourceType &type) const = 0;

    /**
     * @brief Creates a FinishSetupSource task.
     *
     * @param[in] context       : The GstGenericPlayer context
     * @param[in] player        : The GstGenericPlayer instance
     *
     * @retval the new FinishSetupSource task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createFinishSetupSource(GenericPlayerContext &context,
                                                                 IGstGenericPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a HandleBusMessage task.
     *
     * @param[in] context    : The GstGenericPlayer context
     * @param[in] message    : The message to be handled.
     *
     * @retval the new HandleBusMessage task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createHandleBusMessage(GenericPlayerContext &context,
                                                                IGstGenericPlayerPrivate &player,
                                                                GstMessage *message) const = 0;

    /**
     * @brief Creates a NeedData task.
     *
     * @param[in] context : The GstGenericPlayer context
     * @param[in] src     : The source, which reports need data.
     *
     * @retval the new NeedData task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createNeedData(GenericPlayerContext &context, GstAppSrc *src) const = 0;

    /**
     * @brief Creates a Pause task.
     *
     * @param[in] context : The GstGenericPlayer context
     * @param[in] player  : The GstGenericPlayer instance
     *
     * @retval the new Pause task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createPause(GenericPlayerContext &context,
                                                     IGstGenericPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a Play task.
     *
     * @param[in] player        : The GstGenericPlayer instance
     *
     * @retval the new Play task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createPlay(IGstGenericPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a ReadShmDataAndAttachSamples task.
     *
     * @param[in] context       : The GstGenericPlayer context
     * @param[in] player        : The GstGenericPlayer instance
     * @param[in] dataReader    : The shared memory data reader
     *
     * @retval the new ReadShmDataAndAttachSamples task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createReadShmDataAndAttachSamples(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                      const std::shared_ptr<IDataReader> &dataReader) const = 0;

    /**
     * @brief Creates a Remove Source task.
     *
     * @param[in] context : The GstPlayer context
     * @param[in] player  : The GstGenericPlayer instance
     * @param[in] type    : The media source type to remove
     *
     * @retval the new Remove Source task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createRemoveSource(GenericPlayerContext &context,
                                                            IGstGenericPlayerPrivate &player,
                                                            const firebolt::rialto::MediaSourceType &type) const = 0;

    /**
     * @brief Creates a ReportPosition task.
     *
     * @param[in] context       : The GstGenericPlayer context
     *
     * @retval the new ReportPosition task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createReportPosition(GenericPlayerContext &context) const = 0;

    /**
     * @brief Creates a CheckAudioUnderflow task.
     *
     * @param[in] context       : The GstGenericPlayer context
     * @param[in] player        : The GstGenericPlayer instance
     *
     * @retval the new CheckAudioUnderflow task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createCheckAudioUnderflow(GenericPlayerContext &context,
                                                                   IGstGenericPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a SetPlaybackRate task.
     *
     * @param[in] context   : The GstGenericPlayer context
     * @param[in] rate      : The new playback rate.
     *
     * @retval the new SetPlaybackRate task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetPlaybackRate(GenericPlayerContext &context, double rate) const = 0;

    /**
     * @brief Creates a SetPosition task.
     *
     * @param[in] context     : The GstGenericPlayer context
     * @param[in] player      : The GstGenericPlayer instance
     * @param[in] position    : The position to be set
     *
     * @retval the new SetPosition task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createSetPosition(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, std::int64_t position) const = 0;

    /**
     * @brief Creates a SetupElement task.
     *
     * @param[in] context    : The GstGenericPlayer context
     * @param[in] player     : The GstGenericPlayer instance
     * @param[in] element    : The element to be setup.
     *
     * @retval the new SetupElement task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createSetupElement(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, GstElement *element) const = 0;

    /**
     * @brief Creates a SetupSource task.
     *
     * @param[in] context   : The GstGenericPlayer context
     * @param[in] player    : The GstGenericPlayer instance
     * @param[in] source    : The source to be setup.
     *
     * @retval the new SetupSource task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createSetupSource(GenericPlayerContext &context, IGstGenericPlayerPrivate &player, GstElement *source) const = 0;

    /**
     * @brief Creates a SetVideoGeometry task.
     *
     * @param[in] context      : The GstGenericPlayer context
     * @param[in] player       : The GstGenericPlayer instance
     * @param[in] rectangle    : The video geometry data.
     *
     * @retval the new SetVideoGeometry task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetVideoGeometry(GenericPlayerContext &context,
                                                                IGstGenericPlayerPrivate &player,
                                                                const Rectangle &rectangle) const = 0;

    /**
     * @brief Creates a SetVolume task.
     *
     * @param[in] context       : The GstGenericPlayer context
     * @param[in] volume        : The volume to be set
     *
     * @retval the new SetVolume task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetVolume(GenericPlayerContext &context, double volume) const = 0;

    /**
     * @brief Creates a SetMute task.
     *
     * @param[in] context       : The GstGenericPlayer context
     * @param[in] mute          : The mute state to be set
     *
     * @retval the new SetMute task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetMute(GenericPlayerContext &context, const MediaSourceType &mediaSourceType, bool mute) const = 0;

    /**
     * @brief Creates a Shutdown task.
     *
     * @param[in] context       : The GstGenericPlayer context
     *
     * @retval the new Shutdown task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createShutdown(IGstGenericPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a Stop task.
     *
     * @param[in] context    : The GstGenericPlayer context
     * @param[in] player     : The GstGenericPlayer instance
     *
     * @retval the new Stop  task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createStop(GenericPlayerContext &context,
                                                    IGstGenericPlayerPrivate &player) const = 0;

    /**
     * @brief Creates an Underflow task.
     *
     * @param[in] context          : The GstGenericPlayer context
     * @param[in] player           : The GstPlayer instance
     * @param[in] underflowEnabled : The underflow enabled flag (audio or video).
     *
     * @retval the new Underflow task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createUnderflow(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                                                         bool underflowEnabled, MediaSourceType sourceType) const = 0;

    /**
     * @brief Creates an UpdatePlaybackGroup task.
     *
     * @param[in] context       : The GstGenericPlayer context
     * @param[in] typefind      : The typefind element.
     * @param[in] caps          : The GstCaps of added element
     *
     * @retval the new UpdatePlaybackGroup task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createUpdatePlaybackGroup(GenericPlayerContext &context, GstElement *typefind,
                                                                   const GstCaps *caps) const = 0;

    virtual std::unique_ptr<IPlayerTask> createRenderFrame(GenericPlayerContext &context,
                                                           IGstGenericPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a Ping task.
     *
     * @param[in] heartbeatHandler       : The HeartbeatHandler instance
     *
     * @retval the new Ping task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createPing(std::unique_ptr<IHeartbeatHandler> &&heartbeatHandler) const = 0;

    /**
     * @brief Creates a Flush task.
     *
     * @param[in] context   : The GstPlayer context
     * @param[in] type      : The media source type to flush
     * @param[in] resetTime : True if time should be reset
     *
     * @retval the new Flush task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createFlush(GenericPlayerContext &context, const firebolt::rialto::MediaSourceType &type, bool resetTime) const = 0;

    /**
     * @brief Creates a SetSourcePosition task.
     *
     * @param[in] context   : The GstPlayer context
     * @param[in] player     : The GstGenericPlayer instance
     * @param[in] type      : The media source type to set position
     * @param[in] position  : The new source position
     *
     * @retval the new SetSourcePosition task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetSourcePosition(GenericPlayerContext &context,
                                                                 IGstGenericPlayerPrivate &player,
                                                                 const firebolt::rialto::MediaSourceType &type,
                                                                 std::int64_t position) const = 0;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_GENERIC_PLAYER_TASK_FACTORY_H_
