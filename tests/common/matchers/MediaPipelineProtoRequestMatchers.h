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
#include "mediapipelinecapabilitiesmodule.pb.h"
#include "mediapipelinemodule.pb.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
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

inline bool checkAttachSourceCodecData(const firebolt::rialto::AttachSourceRequest *request,
                                       const std::shared_ptr<firebolt::rialto::CodecData> &expectedCodecData)
{
    bool checkCodec = false;
    std::shared_ptr<firebolt::rialto::CodecData> codecDataFromReq;
    if (request->has_codec_data())
    {
        codecDataFromReq = std::make_shared<firebolt::rialto::CodecData>();
        codecDataFromReq->data =
            std::vector<std::uint8_t>(request->codec_data().data().begin(), request->codec_data().data().end());
        if (request->codec_data().type() == AttachSourceRequest_CodecData_Type_STRING)
        {
            codecDataFromReq->type = firebolt::rialto::CodecDataType::STRING;
        }
        else
        {
            codecDataFromReq->type = firebolt::rialto::CodecDataType::BUFFER;
        }
    }

    if (codecDataFromReq && expectedCodecData)
    {
        checkCodec = codecDataFromReq->data == expectedCodecData->data &&
                     codecDataFromReq->type == expectedCodecData->type;
    }
    else
    {
        checkCodec = codecDataFromReq == expectedCodecData;
    }

    return checkCodec;
}

MATCHER_P9(attachSourceRequestMatcherAudio, sessionId, mimeType, hasDrm, alignment, numberOfChannels, sampleRate,
           codecSpecificConfig, codecData, streamFormat, "")
{
    const ::firebolt::rialto::AttachSourceRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);
    bool checkCodec = checkAttachSourceCodecData(kRequest, codecData);

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
    bool checkCodec = checkAttachSourceCodecData(kRequest, codecData);

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
    bool checkCodec = checkAttachSourceCodecData(kRequest, codecData);

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

MATCHER_P4(attachSourceRequestMatcherSubtitle, sessionId, mimeType, hasDrm, textTrackIdentifier, "")
{
    const ::firebolt::rialto::AttachSourceRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);

    return ((kRequest->session_id() == sessionId) && (kRequest->mime_type() == mimeType) &&
            (kRequest->has_drm() == hasDrm) && (kRequest->text_track_identifier() == textTrackIdentifier));
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

MATCHER_P2(setPlaybackRateRequestMatcher, sessionId, rate, "")
{
    const ::firebolt::rialto::SetPlaybackRateRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetPlaybackRateRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->rate() == rate));
}

MATCHER_P2(setPositionRequestMatcher, sessionId, position, "")
{
    const ::firebolt::rialto::SetPositionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetPositionRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->position() == position));
}

MATCHER_P4(setSourcePositionRequestMatcher, sessionId, sourceId, position, resetTime, "")
{
    const ::firebolt::rialto::SetSourcePositionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetSourcePositionRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->position() == position) &&
            (kRequest->source_id() == sourceId) && (kRequest->reset_time() == resetTime));
}

MATCHER_P2(setVolumeRequestMatcher, sessionId, volume, "")
{
    const ::firebolt::rialto::SetVolumeRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetVolumeRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->volume() == volume));
}

MATCHER_P(getVolumeRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::GetVolumeRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetVolumeRequest *>(arg);
    return ((kRequest->session_id() == sessionId));
}

MATCHER_P3(setMuteRequestMatcher, sessionId, sourceId, mute, "")
{
    const ::firebolt::rialto::SetMuteRequest *kRequest = dynamic_cast<const ::firebolt::rialto::SetMuteRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->source_id() == sourceId) && (kRequest->mute() == mute));
}

MATCHER_P2(getMuteRequestMatcher, sessionId, sourceId, "")
{
    const ::firebolt::rialto::GetMuteRequest *kRequest = dynamic_cast<const ::firebolt::rialto::GetMuteRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->source_id() == sourceId));
}

MATCHER_P5(setVideoWindowRequestMatcher, sessionId, x, y, width, height, "")
{
    const ::firebolt::rialto::SetVideoWindowRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetVideoWindowRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->x() == x) && (kRequest->y() == y) &&
            (kRequest->width() == width) && (kRequest->height() == height));
}

MATCHER_P(renderFrameRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::RenderFrameRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::RenderFrameRequest *>(arg);
    return ((kRequest->session_id() == sessionId));
}

MATCHER_P(getPositionRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::GetPositionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetPositionRequest *>(arg);
    return ((kRequest->session_id() == sessionId));
}

MATCHER_P2(setImmediateOutputRequestMatcher, sessionId, sourceId, "")
{
    const ::firebolt::rialto::SetImmediateOutputRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetImmediateOutputRequest *>(arg);
    return (kRequest->session_id() == sessionId) && (kRequest->source_id() == sourceId);
}

MATCHER_P2(getImmediateOutputRequestMatcher, sessionId, sourceId, "")
{
    const ::firebolt::rialto::GetImmediateOutputRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetImmediateOutputRequest *>(arg);
    return (kRequest->session_id() == sessionId) && (kRequest->source_id() == sourceId);
}

MATCHER_P2(getStatsRequestMatcher, sessionId, sourceId, "")
{
    const ::firebolt::rialto::GetStatsRequest *kRequest = dynamic_cast<const ::firebolt::rialto::GetStatsRequest *>(arg);
    return (kRequest->session_id() == sessionId) && (kRequest->source_id() == sourceId);
}

MATCHER_P(getSupportedMimeTypesRequestMatcher, sourceType, "")
{
    const ::firebolt::rialto::GetSupportedMimeTypesRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetSupportedMimeTypesRequest *>(arg);
    return (kRequest->media_type() == static_cast<firebolt::rialto::ProtoMediaSourceType>(sourceType));
}

MATCHER_P(isMimeTypeSupportedRequestMatcher, mimeType, "")
{
    const ::firebolt::rialto::IsMimeTypeSupportedRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::IsMimeTypeSupportedRequest *>(arg);
    return (kRequest->mime_type() == mimeType);
}

MATCHER_P2(getSupportedPropertiesRequestMatcher, mediaType, propertyNames, "")
{
    const ::firebolt::rialto::GetSupportedPropertiesRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetSupportedPropertiesRequest *>(arg);
    std::vector<std::string> tmp{kRequest->property_names().begin(), kRequest->property_names().end()};
    return (kRequest->media_type() == static_cast<firebolt::rialto::ProtoMediaSourceType>(mediaType) &&
            tmp == propertyNames);
}

MATCHER_P3(flushRequestMatcher, sessionId, sourceId, resetTime, "")
{
    const ::firebolt::rialto::FlushRequest *kRequest = dynamic_cast<const ::firebolt::rialto::FlushRequest *>(arg);
    return (kRequest->session_id() == sessionId) && (kRequest->source_id() == sourceId) &&
           (kRequest->reset_time() == resetTime);
}

MATCHER_P5(processAudioGapRequestMatcher, sessionId, position, duration, discontinuityGap, isAudioAac, "")
{
    const ::firebolt::rialto::ProcessAudioGapRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::ProcessAudioGapRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->position() == position) &&
            (kRequest->duration() == duration) && (kRequest->discontinuity_gap() == discontinuityGap) &&
            (kRequest->audio_aac() == isAudioAac));
}

MATCHER_P2(setTextTrackIdentifierRequestMatcher, sessionId, textTrackIdentifier, "")
{
    const ::firebolt::rialto::SetTextTrackIdentifierRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetTextTrackIdentifierRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->text_track_identifier() == textTrackIdentifier));
}

MATCHER_P(getTextTrackIdentifierRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::GetTextTrackIdentifierRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetTextTrackIdentifierRequest *>(arg);
    return (kRequest->session_id() == sessionId);
}
#endif // MEDIA_PIPELINE_PROTO_REQUEST_MATCHERS_H_
