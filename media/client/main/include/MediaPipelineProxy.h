/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_PROXY_H_
#define FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_PROXY_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "MediaPipeline.h"

namespace firebolt::rialto::client
{
class MediaPipelineProxy : public IMediaPipelineAndIControlClient
{
public:
    MediaPipelineProxy(const std::shared_ptr<IMediaPipelineAndIControlClient> &mediaPipeline,
                       client::IClientController &clientController);
    virtual ~MediaPipelineProxy();

    std::weak_ptr<IMediaPipelineClient> getClient() override { return m_mediaPipeline->getClient(); }

    bool load(MediaType type, const std::string &mimeType, const std::string &url) override
    {
        return m_mediaPipeline->load(type, mimeType, url);
    }

    bool attachSource(const std::unique_ptr<MediaSource> &source) override
    {
        return m_mediaPipeline->attachSource(source);
    }

    bool removeSource(int32_t id) override { return m_mediaPipeline->removeSource(id); }

    bool allSourcesAttached() override { return m_mediaPipeline->allSourcesAttached(); }

    bool play() override { return m_mediaPipeline->play(); }

    bool pause() override { return m_mediaPipeline->pause(); }

    bool stop() override { return m_mediaPipeline->stop(); }

    bool setPlaybackRate(double rate) override { return m_mediaPipeline->setPlaybackRate(rate); }

    bool setPosition(int64_t position) override { return m_mediaPipeline->setPosition(position); }

    bool getPosition(int64_t &position) override { return m_mediaPipeline->getPosition(position); }

    bool setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override
    {
        return m_mediaPipeline->setVideoWindow(x, y, width, height);
    }

    bool haveData(MediaSourceStatus status, uint32_t needDataRequestId) override
    {
        return m_mediaPipeline->haveData(status, needDataRequestId);
    }

    AddSegmentStatus addSegment(uint32_t needDataRequestId, const std::unique_ptr<MediaSegment> &mediaSegment) override
    {
        return m_mediaPipeline->addSegment(needDataRequestId, mediaSegment);
    }

    bool renderFrame() override { return m_mediaPipeline->renderFrame(); }

    bool setVolume(double volume) override { return m_mediaPipeline->setVolume(volume); }

    bool getVolume(double &volume) override { return m_mediaPipeline->getVolume(volume); }

    bool setMute(int32_t sourceId, bool mute) override { return m_mediaPipeline->setMute(sourceId, mute); }

    bool getMute(int32_t sourceId, bool &mute) override { return m_mediaPipeline->getMute(sourceId, mute); }

    bool flush(int32_t sourceId, bool resetTime) override { return m_mediaPipeline->flush(sourceId, resetTime); }

    bool setSourcePosition(int32_t sourceId, int64_t position) override
    {
        return m_mediaPipeline->setSourcePosition(sourceId, position);
    }

    void notifyApplicationState(ApplicationState state) override { m_mediaPipeline->notifyApplicationState(state); }

private:
    std::shared_ptr<IMediaPipelineAndIControlClient> m_mediaPipeline;
    client::IClientController &m_clientController;
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_PROXY_H_
