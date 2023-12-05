/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef MEDIA_PIPELINE_PROTO_REQUEST_MATCHERS_H_
#define MEDIA_PIPELINE_PROTO_REQUEST_MATCHERS_H_

#include "MediaCommon.h"
#include "mediapipelinemodule.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

MATCHER_P2(createSessionRequestMatcher, maxWidth, maxHeight, "")
{
    const ::firebolt::rialto::CreateSessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::CreateSessionRequest *>(arg);
    return ((kRequest->max_width() == maxWidth) && (kRequest->max_height() == maxHeight));
}

MATCHER_P4(loadRequestMatcher, sessionId, type, mimeType, url, "")
{
    const ::firebolt::rialto::LoadRequest *kRequest = dynamic_cast<const ::firebolt::rialto::LoadRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->type() == type) &&
            (kRequest->mime_type() == mimeType) && (kRequest->url() == url));
}

MATCHER_P(playRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::PlayRequest *kRequest = dynamic_cast<const ::firebolt::rialto::PlayRequest *>(arg);
    return (kRequest->session_id() == sessionId);
}

MATCHER_P(pauseRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::PauseRequest *kRequest = dynamic_cast<const ::firebolt::rialto::PauseRequest *>(arg);
    return (kRequest->session_id() == sessionId);
}

MATCHER_P9(attachSourceRequestMatcherAudio, sessionId, mimeType, hasDrm, alignment, numberOfChannels, sampleRate,
           codecSpecificConfig, codecData, streamFormat, "")
{
    const ::firebolt::rialto::AttachSourceRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);
    std::shared_ptr<firebolt::rialto::CodecData> codecDataFromReq{};
    if (kRequest->has_codec_data())
    {
        codecDataFromReq = std::make_shared<firebolt::rialto::CodecData>();
        codecDataFromReq->data =
            std::vector<std::uint8_t>(kRequest->codec_data().data().begin(), kRequest->codec_data().data().end());
        if (kRequest->codec_data().type() == AttachSourceRequest_CodecData_Type_STRING)
        {
            codecDataFromReq->type = firebolt::rialto::CodecDataType::STRING;
        }
        else
        {
            codecDataFromReq->type = firebolt::rialto::CodecDataType::BUFFER;
        }
    }

    bool checkCodec = false;
    if (codecDataFromReq && codecData)
    {
        checkCodec = codecDataFromReq->data == codecData->data && codecDataFromReq->type == codecData->type;
    }
    else
    {
        checkCodec = codecDataFromReq == codecData;
    }

    // Only check optional parameters if the config type is correct
    bool checkAudio = false;
    if (static_cast<const unsigned int>(kRequest->config_type()) ==
        static_cast<const unsigned int>(SourceConfigType::AUDIO))
    {
        if ((kRequest->has_audio_config()) && (kRequest->audio_config().number_of_channels() == numberOfChannels) &&
            (kRequest->audio_config().sample_rate() == sampleRate) &&
            (kRequest->audio_config().codec_specific_config() == codecSpecificConfig))
        {
            checkAudio = true;
        }
    }

    return ((kRequest->session_id() == sessionId) && (kRequest->mime_type() == mimeType) &&
            (kRequest->has_drm() == hasDrm) && (kRequest->stream_format() == streamFormat) && checkCodec && checkAudio);
}

MATCHER_P8(attachSourceRequestMatcherVideo, sessionId, mimeType, hasDrm, width, height, alignment, codecData,
           streamFormat, "")
{
    const ::firebolt::rialto::AttachSourceRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);
    std::shared_ptr<firebolt::rialto::CodecData> codecDataFromReq{};
    if (kRequest->has_codec_data())
    {
        codecDataFromReq = std::make_shared<firebolt::rialto::CodecData>();
        codecDataFromReq->data =
            std::vector<std::uint8_t>(kRequest->codec_data().data().begin(), kRequest->codec_data().data().end());
        if (kRequest->codec_data().type() == AttachSourceRequest_CodecData_Type_STRING)
        {
            codecDataFromReq->type = firebolt::rialto::CodecDataType::STRING;
        }
        else
        {
            codecDataFromReq->type = firebolt::rialto::CodecDataType::BUFFER;
        }
    }

    bool checkCodec = false;
    if (codecDataFromReq && codecData)
    {
        checkCodec = codecDataFromReq->data == codecData->data && codecDataFromReq->type == codecData->type;
    }
    else
    {
        checkCodec = codecDataFromReq == codecData;
    }

    // Only check optional parameters if the config type is correct
    bool checkVideo = false;
    if (static_cast<const unsigned int>(kRequest->config_type()) ==
        static_cast<const unsigned int>(SourceConfigType::VIDEO))
    {
        if ((kRequest->width() == width) && (kRequest->height() == height))
        {
            checkVideo = true;
        }
    }

    return ((kRequest->session_id() == sessionId) && (kRequest->mime_type() == mimeType) &&
            (kRequest->has_drm() == hasDrm) && (kRequest->stream_format() == streamFormat) && checkCodec && checkVideo);
}

MATCHER_P9(attachSourceRequestMatcherDolby, sessionId, mimeType, hasDrm, width, height, alignment, codecData,
           streamFormat, dolbyVisionProfile, "")
{
    const ::firebolt::rialto::AttachSourceRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);
    std::shared_ptr<firebolt::rialto::CodecData> codecDataFromReq{};
    if (kRequest->has_codec_data())
    {
        codecDataFromReq = std::make_shared<firebolt::rialto::CodecData>();
        codecDataFromReq->data =
            std::vector<std::uint8_t>(kRequest->codec_data().data().begin(), kRequest->codec_data().data().end());
        if (kRequest->codec_data().type() == AttachSourceRequest_CodecData_Type_STRING)
        {
            codecDataFromReq->type = firebolt::rialto::CodecDataType::STRING;
        }
        else
        {
            codecDataFromReq->type = firebolt::rialto::CodecDataType::BUFFER;
        }
    }

    bool checkCodec = false;
    if (codecDataFromReq && codecData)
    {
        checkCodec = codecDataFromReq->data == codecData->data && codecDataFromReq->type == codecData->type;
    }
    else
    {
        checkCodec = codecDataFromReq == codecData;
    }

    // Only check optional parameters if the config type is correct
    bool checkDolby = false;
    if (static_cast<const unsigned int>(kRequest->config_type()) ==
        static_cast<const unsigned int>(SourceConfigType::VIDEO_DOLBY_VISION))
    {
        if ((kRequest->width() == width) && (kRequest->height() == height) &&
            (kRequest->dolby_vision_profile() == dolbyVisionProfile))
        {
            checkDolby = true;
        }
    }

    return ((kRequest->session_id() == sessionId) && (kRequest->mime_type() == mimeType) &&
            (kRequest->has_drm() == hasDrm) && (kRequest->stream_format() == streamFormat) && checkCodec && checkDolby);
}

MATCHER_P2(removeSourceRequestMatcher, sessionId, sourceId, "")
{
    const ::firebolt::rialto::RemoveSourceRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::RemoveSourceRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->source_id() == sourceId));
}

MATCHER_P(allSourcesAttachedRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::AllSourcesAttachedRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::AllSourcesAttachedRequest *>(arg);
    return ((kRequest->session_id() == sessionId));
}

MATCHER_P4(haveDataRequestMatcher, sessionId, status, numFrames, requestId, "")
{
    const ::firebolt::rialto::HaveDataRequest *kRequest = dynamic_cast<const ::firebolt::rialto::HaveDataRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->status() == status) &&
            (kRequest->request_id() == requestId) && (kRequest->num_frames() == numFrames));
}

MATCHER_P(stopRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::StopRequest *kRequest = dynamic_cast<const ::firebolt::rialto::StopRequest *>(arg);
    return (kRequest->session_id() == sessionId);
}

MATCHER_P(destroySessionRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::DestroySessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::DestroySessionRequest *>(arg);
    return (kRequest->session_id() == sessionId);
}

#endif // MEDIA_PIPELINE_PROTO_REQUEST_MATCHERS_H_
