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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_VIDEO_FRAME_CAPTURE_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_VIDEO_FRAME_CAPTURE_SERVICE_MOCK_H_

#include "IVideoFrameCaptureService.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::server::service
{
class VideoFrameCaptureServiceMock : public IVideoFrameCaptureService
{
public:
    MOCK_METHOD(VideoFrameCaptureServiceStatus, createCapture,
                (VideoFrameCaptureClientId clientId, std::optional<GraphicsAPIType> api, std::uint32_t &captureId),
                (override));
    MOCK_METHOD(bool, destroyCapture, (VideoFrameCaptureClientId clientId, std::uint32_t captureId), (override));
    MOCK_METHOD(bool, hasCapture, (VideoFrameCaptureClientId clientId, std::uint32_t captureId), (const, override));
    MOCK_METHOD(void, removeClient, (VideoFrameCaptureClientId clientId), (override));
    MOCK_METHOD(bool, bindCapture, (VideoFrameCaptureClientId clientId, std::uint32_t captureId, int sessionId),
                (override));
    MOCK_METHOD(void, unbindCapture, (VideoFrameCaptureClientId clientId, std::uint32_t captureId), (override));
    MOCK_METHOD(void, setBoundCaptureRemovedHandler, (const BoundCaptureRemovedHandler &handler), (override));
    MOCK_METHOD(void, setCaptureRuntime,
                (VideoFrameCaptureClientId clientId, std::uint32_t captureId,
                 const std::shared_ptr<CaptureSessionRuntime> &runtime),
                (override));
    MOCK_METHOD(std::shared_ptr<CaptureSessionRuntime>, captureRuntime,
                (VideoFrameCaptureClientId clientId, std::uint32_t captureId), (const, override));
    MOCK_METHOD(std::shared_ptr<CaptureSessionRuntime>, sessionForRelease,
                (VideoFrameCaptureClientId clientId, std::uint32_t captureId, std::uint32_t poolId), (const, override));
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_VIDEO_FRAME_CAPTURE_SERVICE_MOCK_H_
