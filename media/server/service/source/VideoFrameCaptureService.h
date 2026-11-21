/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_VIDEO_FRAME_CAPTURE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_VIDEO_FRAME_CAPTURE_SERVICE_H_

#include "ICaptureHalLibrary.h"
#include "IVideoFrameCaptureService.h"
#include "VideoFrameCaptureRegistry.h"

#include <map>
#include <memory>
#include <mutex>

namespace firebolt::rialto::server::service
{
class VideoFrameCaptureService : public IVideoFrameCaptureService
{
public:
    /**
     * @param[in] captureHalLibraryFactory : Gives the capture HAL loader, which decides whether capture is available.
     */
    explicit VideoFrameCaptureService(const std::shared_ptr<wrappers::ICaptureHalLibraryFactory> &captureHalLibraryFactory);
    ~VideoFrameCaptureService() override = default;

    VideoFrameCaptureServiceStatus createCapture(VideoFrameCaptureClientId clientId, std::optional<GraphicsAPIType> api,
                                                 std::uint32_t &captureId) override;
    bool destroyCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId) override;
    bool hasCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId) const override;
    void removeClient(VideoFrameCaptureClientId clientId) override;
    bool bindCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId, int sessionId) override;
    void unbindCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId) override;
    void setBoundCaptureRemovedHandler(const BoundCaptureRemovedHandler &handler) override;
    void setCaptureRuntime(VideoFrameCaptureClientId clientId, std::uint32_t captureId,
                           const std::shared_ptr<CaptureSessionRuntime> &runtime) override;
    std::shared_ptr<CaptureSessionRuntime> captureRuntime(VideoFrameCaptureClientId clientId,
                                                          std::uint32_t captureId) const override;

private:
    /** Must be called with m_mutex held. */
    bool isCaptureAvailableLocked();

    std::shared_ptr<wrappers::ICaptureHalLibraryFactory> m_captureHalLibraryFactory;
    std::shared_ptr<wrappers::ICaptureHalLibrary> m_captureHalLibrary;
    mutable std::mutex m_mutex;
    std::map<VideoFrameCaptureClientId, VideoFrameCaptureRegistry> m_registries;
    BoundCaptureRemovedHandler m_boundCaptureRemovedHandler;
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_VIDEO_FRAME_CAPTURE_SERVICE_H_
