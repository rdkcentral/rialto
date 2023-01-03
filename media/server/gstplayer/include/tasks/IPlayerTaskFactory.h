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

#ifndef FIREBOLT_RIALTO_SERVER_I_PLAYER_TASK_FACTORY_H_
#define FIREBOLT_RIALTO_SERVER_I_PLAYER_TASK_FACTORY_H_

#include "IDataReader.h"
#include "IGstPlayerPrivate.h"
#include "IMediaPipeline.h"
#include "IPlayerTask.h"
#include "MediaCommon.h"
#include "PlayerContext.h"
#include <cstdint>
#include <gst/app/gstappsrc.h>
#include <memory>

namespace firebolt::rialto::server
{
/**
 * @brief IPlayerTaskFactory factory class, returns a concrete implementation of IPlayerTask
 */
class IPlayerTaskFactory
{
public:
    IPlayerTaskFactory() = default;
    virtual ~IPlayerTaskFactory() = default;

    /**
     * @brief Creates a AttachSamples task.
     *
     * @param[in] context       : The GstPlayer context
     * @param[in] player        : The GstPlayer instance
     * @param[in] mediaSegments : The media segments to attach
     *
     * @retval the new AttachSamples task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createAttachSamples(PlayerContext &context, IGstPlayerPrivate &player,
                        const IMediaPipeline::MediaSegmentVector &mediaSegments) const = 0;

    /**
     * @brief Creates a AttachSource task.
     *
     * @param[in] context   : The GstPlayer context
     * @param[in] source    : The source to attach.
     *
     * @retval the new AttachSource task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createAttachSource(PlayerContext &context, const std::unique_ptr<IMediaPipeline::MediaSource> &source) const = 0;

    /**
     * @brief Creates a EnoughData task.
     *
     * @param[in] context : The GstPlayer context
     * @param[in] src     : The source, which reports enough data.
     *
     * @retval the new EnoughData task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createEnoughData(PlayerContext &context, GstAppSrc *src) const = 0;

    /**
     * @brief Creates a Eos task.
     *
     * @param[in] context : The GstPlayer context
     * @param[in] type    : The media source type, which reports eos.
     *
     * @retval the new Eos task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createEos(PlayerContext &context, IGstPlayerPrivate &player,
                                                   const firebolt::rialto::MediaSourceType &type) const = 0;

    /**
     * @brief Creates a FinishSetupSource task.
     *
     * @param[in] context       : The GstPlayer context
     * @param[in] player        : The GstPlayer instance
     *
     * @retval the new FinishSetupSource task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createFinishSetupSource(PlayerContext &context,
                                                                 IGstPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a HandleBusMessage task.
     *
     * @param[in] context    : The GstPlayer context
     * @param[in] message    : The message to be handled.
     *
     * @retval the new HandleBusMessage task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createHandleBusMessage(PlayerContext &context, IGstPlayerPrivate &player,
                                                                GstMessage *message) const = 0;

    /**
     * @brief Creates a NeedData task.
     *
     * @param[in] context : The GstPlayer context
     * @param[in] src     : The source, which reports need data.
     *
     * @retval the new NeedData task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createNeedData(PlayerContext &context, GstAppSrc *src) const = 0;

    /**
     * @brief Creates a Pause task.
     *
     * @param[in] player        : The GstPlayer instance
     *
     * @retval the new Pause task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createPause(IGstPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a Play task.
     *
     * @param[in] player        : The GstPlayer instance
     *
     * @retval the new Play task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createPlay(IGstPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a ReadShmDataAndAttachSamples task.
     *
     * @param[in] context       : The GstPlayer context
     * @param[in] player        : The GstPlayer instance
     * @param[in] dataReader    : The shared memory data reader
     *
     * @retval the new ReadShmDataAndAttachSamples task instance.
     */
    virtual std::unique_ptr<IPlayerTask>
    createReadShmDataAndAttachSamples(PlayerContext &context, IGstPlayerPrivate &player,
                                      const std::shared_ptr<IDataReader> &dataReader) const = 0;

    /**
     * @brief Creates a ReportPosition task.
     *
     * @param[in] context       : The GstPlayer context
     *
     * @retval the new ReportPosition task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createReportPosition(PlayerContext &context) const = 0;

    /**
     * @brief Creates a CheckAudioUnderflow task.
     *
     * @param[in] context       : The GstPlayer context
     * @param[in] player        : The GstPlayer instance
     *
     * @retval the new CheckAudioUnderflow task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createCheckAudioUnderflow(PlayerContext &context,
                                                                   IGstPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a SetPlaybackRate task.
     *
     * @param[in] context   : The GstPlayer context
     * @param[in] rate      : The new playback rate.
     *
     * @retval the new SetPlaybackRate task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetPlaybackRate(PlayerContext &context, double rate) const = 0;

    /**
     * @brief Creates a SetPosition task.
     *
     * @param[in] context     : The GstPlayer context
     * @param[in] player      : The GstPlayer instance
     * @param[in] position    : The position to be set
     *
     * @retval the new SetPosition task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetPosition(PlayerContext &context, IGstPlayerPrivate &player,
                                                           std::int64_t position) const = 0;

    /**
     * @brief Creates a SetupElement task.
     *
     * @param[in] context    : The GstPlayer context
     * @param[in] player     : The GstPlayer instance
     * @param[in] element    : The element to be setup.
     *
     * @retval the new SetupElement task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetupElement(PlayerContext &context, IGstPlayerPrivate &player,
                                                            GstElement *element) const = 0;

    /**
     * @brief Creates a SetupSource task.
     *
     * @param[in] context   : The GstPlayer context
     * @param[in] player    : The GstPlayer instance
     * @param[in] source    : The source to be setup.
     *
     * @retval the new SetupSource task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetupSource(PlayerContext &context, IGstPlayerPrivate &player,
                                                           GstElement *source) const = 0;

    /**
     * @brief Creates a SetVideoGeometry task.
     *
     * @param[in] context      : The GstPlayer context
     * @param[in] player       : The GstPlayer instance
     * @param[in] rectangle    : The video geometry data.
     *
     * @retval the new SetVideoGeometry task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetVideoGeometry(PlayerContext &context, IGstPlayerPrivate &player,
                                                                const Rectangle &rectangle) const = 0;

    /**
     * @brief Creates a SetVolume task.
     *
     * @param[in] context       : The GstPlayer context
     * @param[in] volume        : The volume to be set
     *
     * @retval the new SetVolume task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createSetVolume(PlayerContext &context, double volume) const = 0;

    /**
     * @brief Creates a Shutdown task.
     *
     * @param[in] context       : The GstPlayer context
     *
     * @retval the new Shutdown task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createShutdown(IGstPlayerPrivate &player) const = 0;

    /**
     * @brief Creates a Stop task.
     *
     * @param[in] context    : The GstPlayer context
     * @param[in] player     : The GstPlayer instance
     *
     * @retval the new Stop  task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createStop(PlayerContext &context, IGstPlayerPrivate &player) const = 0;

    /**
     * @brief Creates an Underflow task.
     *
     * @param[in] player          : The GstPlayer instance
     * @param[in] underflowFlag   : The underflow flag (audio or video).
     *
     * @retval the new Underflow task instance.
     */
    virtual std::unique_ptr<IPlayerTask> createUnderflow(IGstPlayerPrivate &player, bool &underflowFlag) const = 0;

    virtual std::unique_ptr<IPlayerTask> createRenderFrame(PlayerContext &context) const = 0;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_PLAYER_TASK_FACTORY_H_
