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

#ifndef VIDEO_FRAME_CAPTURE_MODULE_MOCK_H_
#define VIDEO_FRAME_CAPTURE_MODULE_MOCK_H_

#include "videoframecapturemodule.pb.h"
#include <gmock/gmock.h>

class VideoFrameCaptureModuleMock : public ::firebolt::rialto::VideoFrameCaptureModule
{
public:
    MOCK_METHOD(void, createVideoFrameCapture,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::CreateVideoFrameCaptureRequest *request,
                 ::firebolt::rialto::CreateVideoFrameCaptureResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, acquireCurrentFrame,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::AcquireCurrentVideoFrameRequest *request,
                 ::firebolt::rialto::AcquireCurrentVideoFrameResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, releaseFrame,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::ReleaseVideoFrameRequest *request,
                 ::firebolt::rialto::ReleaseVideoFrameResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getPool,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetVideoFrameCapturePoolRequest *request,
                 ::firebolt::rialto::GetVideoFrameCapturePoolResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, destroyVideoFrameCapture,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::DestroyVideoFrameCaptureRequest *request,
                 ::firebolt::rialto::DestroyVideoFrameCaptureResponse *response, ::google::protobuf::Closure *done));
};

#endif // VIDEO_FRAME_CAPTURE_MODULE_MOCK_H_
