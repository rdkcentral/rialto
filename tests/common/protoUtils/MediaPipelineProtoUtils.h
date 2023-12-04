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

#ifndef MEDIA_PIPELINE_PROTO_UTILS_H_
#define MEDIA_PIPELINE_PROTO_UTILS_H_

#include "MediaCommon.h"
#include "mediapipelinemodule.pb.h"

firebolt::rialto::LoadRequest_MediaType convertMediaType(const firebolt::rialto::MediaType &kMediaType)
{
    switch (kMediaType)
    {
    case firebolt::rialto::MediaType::UNKNOWN:
    {
        return firebolt::rialto::LoadRequest_MediaType::LoadRequest_MediaType_UNKNOWN;
    }
    case firebolt::rialto::MediaType::MSE:
    {
        return firebolt::rialto::LoadRequest_MediaType::LoadRequest_MediaType_MSE;
    }
    }
    return firebolt::rialto::LoadRequest_MediaType::LoadRequest_MediaType_UNKNOWN;
}

firebolt::rialto::AttachSourceRequest_StreamFormat convertStreamFormat(const firebolt::rialto::StreamFormat &streamFormat)
{
    switch (streamFormat)
    {
    case firebolt::rialto::StreamFormat::UNDEFINED:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_UNDEFINED;
    }
    case firebolt::rialto::StreamFormat::RAW:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW;
    }
    case firebolt::rialto::StreamFormat::AVC:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_AVC;
    }
    case firebolt::rialto::StreamFormat::BYTE_STREAM:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_BYTE_STREAM;
    }
    }
    return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_UNDEFINED;
}

firebolt::rialto::HaveDataRequest_MediaSourceStatus
convertMediaSourceStatus(const firebolt::rialto::MediaSourceStatus &status)
{
    firebolt::rialto::HaveDataRequest_MediaSourceStatus protoMediaSourceStatus =
        firebolt::rialto::HaveDataRequest_MediaSourceStatus_UNKNOWN;
    switch (status)
    {
    case firebolt::rialto::MediaSourceStatus::OK:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_OK;
        break;
    case firebolt::rialto::MediaSourceStatus::EOS:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_EOS;
        break;
    case firebolt::rialto::MediaSourceStatus::ERROR:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_ERROR;
        break;
    case firebolt::rialto::MediaSourceStatus::CODEC_CHANGED:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_CODEC_CHANGED;
        break;
    case firebolt::rialto::MediaSourceStatus::NO_AVAILABLE_SAMPLES:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_NO_AVAILABLE_SAMPLES;
        break;
    default:
        break;
    }

    return protoMediaSourceStatus;
}

#endif // MEDIA_PIPELINE_PROTO_UTILS_H_
