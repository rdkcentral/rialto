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

#ifndef FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_ATTACH_SAMPLES_H_
#define FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_ATTACH_SAMPLES_H_

#include "GenericPlayerContext.h"
#include "IGstGenericPlayerPrivate.h"
#include "IMediaPipeline.h"
#include "IPlayerTask.h"
#include <gst/gst.h>
#include <memory>
#include <vector>

namespace firebolt::rialto::server::tasks::generic
{
class AttachSamples : public IPlayerTask
{
public:
    AttachSamples(GenericPlayerContext &context, IGstGenericPlayerPrivate &player,
                  const IMediaPipeline::MediaSegmentVector &mediaSegments);
    ~AttachSamples() override;
    void execute() const override;

private:
    struct AudioData
    {
        GstBuffer *buffer;
        int32_t rate;
        int32_t channels;
        std::shared_ptr<std::vector<std::uint8_t>> codecData;
    };
    struct VideoData
    {
        GstBuffer *buffer;
        int32_t width;
        int32_t height;
        Fraction frameRate;
        std::shared_ptr<std::vector<std::uint8_t>> codecData;
    };

    GenericPlayerContext &m_context;
    IGstGenericPlayerPrivate &m_player;
    std::vector<AudioData> m_audioData;
    std::vector<VideoData> m_videoData;
};
} // namespace firebolt::rialto::server::tasks::generic

#endif // FIREBOLT_RIALTO_SERVER_TASKS_GENERIC_ATTACH_SAMPLES_H_
