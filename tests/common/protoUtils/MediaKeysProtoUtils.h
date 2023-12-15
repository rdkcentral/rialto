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

#ifndef MEDIA_KEYS_PROTO_UTILS_H_
#define MEDIA_KEYS_PROTO_UTILS_H_

#include "MediaCommon.h"
#include "mediakeysmodule.pb.h"

inline firebolt::rialto::KeyStatusesChangedEvent_KeyStatus convertKeyStatus(const firebolt::rialto::KeyStatus &keyStatus)
{
    switch (keyStatus)
    {
    case firebolt::rialto::KeyStatus::USABLE:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_USABLE;
    }
    case firebolt::rialto::KeyStatus::EXPIRED:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_EXPIRED;
    }
    case firebolt::rialto::KeyStatus::OUTPUT_RESTRICTED:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_OUTPUT_RESTRICTED;
    }
    case firebolt::rialto::KeyStatus::PENDING:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_PENDING;
    }
    case firebolt::rialto::KeyStatus::INTERNAL_ERROR:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_INTERNAL_ERROR;
    }
    case firebolt::rialto::KeyStatus::RELEASED:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_RELEASED;
    }
    }
    return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_INTERNAL_ERROR;
}

inline firebolt::rialto::MediaKeyErrorStatus
convertMediaKeyErrorStatus(const firebolt::rialto::ProtoMediaKeyErrorStatus &errorStatus)
{
    switch (errorStatus)
    {
    case firebolt::rialto::ProtoMediaKeyErrorStatus::OK:
    {
        return firebolt::rialto::MediaKeyErrorStatus::OK;
    }
    case firebolt::rialto::ProtoMediaKeyErrorStatus::BAD_SESSION_ID:
    {
        return firebolt::rialto::MediaKeyErrorStatus::BAD_SESSION_ID;
    }
    case firebolt::rialto::ProtoMediaKeyErrorStatus::NOT_SUPPORTED:
    {
        return firebolt::rialto::MediaKeyErrorStatus::NOT_SUPPORTED;
    }
    case firebolt::rialto::ProtoMediaKeyErrorStatus::INVALID_STATE:
    {
        return firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE;
    }
    case firebolt::rialto::ProtoMediaKeyErrorStatus::FAIL:
    {
        return firebolt::rialto::MediaKeyErrorStatus::FAIL;
    }
    }
    return firebolt::rialto::MediaKeyErrorStatus::FAIL;
}

inline firebolt::rialto::ProtoMediaKeyErrorStatus
convertMediaKeyErrorStatus(const firebolt::rialto::MediaKeyErrorStatus &errorStatus)
{
    switch (errorStatus)
    {
    case firebolt::rialto::MediaKeyErrorStatus::OK:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::OK;
    }
    case firebolt::rialto::MediaKeyErrorStatus::BAD_SESSION_ID:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::BAD_SESSION_ID;
    }
    case firebolt::rialto::MediaKeyErrorStatus::NOT_SUPPORTED:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::NOT_SUPPORTED;
    }
    case firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::INVALID_STATE;
    }
    case firebolt::rialto::MediaKeyErrorStatus::FAIL:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::FAIL;
    }
    }
    return firebolt::rialto::ProtoMediaKeyErrorStatus::FAIL;
}

inline firebolt::rialto::CreateKeySessionRequest_KeySessionType
convertKeySessionType(const firebolt::rialto::KeySessionType &kKeySessionType)
{
    switch (kKeySessionType)
    {
    case firebolt::rialto::KeySessionType::TEMPORARY:
        return firebolt::rialto::CreateKeySessionRequest_KeySessionType::CreateKeySessionRequest_KeySessionType_TEMPORARY;
    case firebolt::rialto::KeySessionType::PERSISTENT_LICENCE:
        return firebolt::rialto::CreateKeySessionRequest_KeySessionType::CreateKeySessionRequest_KeySessionType_PERSISTENT_LICENCE;
    case firebolt::rialto::KeySessionType::PERSISTENT_RELEASE_MESSAGE:
        return firebolt::rialto::CreateKeySessionRequest_KeySessionType::CreateKeySessionRequest_KeySessionType_PERSISTENT_RELEASE_MESSAGE;
    default:
        return firebolt::rialto::CreateKeySessionRequest_KeySessionType::CreateKeySessionRequest_KeySessionType_UNKNOWN;
    }
}

inline firebolt::rialto::GenerateRequestRequest_InitDataType convertInitDataType(firebolt::rialto::InitDataType kInitDataType)
{
    switch (kInitDataType)
    {
    case firebolt::rialto::InitDataType::CENC:
        return firebolt::rialto::GenerateRequestRequest_InitDataType::GenerateRequestRequest_InitDataType_CENC;
    case firebolt::rialto::InitDataType::KEY_IDS:
        return firebolt::rialto::GenerateRequestRequest_InitDataType::GenerateRequestRequest_InitDataType_KEY_IDS;
    case firebolt::rialto::InitDataType::WEBM:
        return firebolt::rialto::GenerateRequestRequest_InitDataType::GenerateRequestRequest_InitDataType_WEBM;
    case firebolt::rialto::InitDataType::DRMHEADER:
        return firebolt::rialto::GenerateRequestRequest_InitDataType::GenerateRequestRequest_InitDataType_DRMHEADER;
    default:
        return firebolt::rialto::GenerateRequestRequest_InitDataType::GenerateRequestRequest_InitDataType_UNKNOWN;
    }
}

inline firebolt::rialto::KeyStatus convertKeyStatus(const firebolt::rialto::KeyStatusesChangedEvent_KeyStatus &keyStatus)
{
    switch (keyStatus)
    {
    case firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_USABLE:
    {
        return firebolt::rialto::KeyStatus::USABLE;
    }
    case firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_EXPIRED:
    {
        return firebolt::rialto::KeyStatus::EXPIRED;
    }
    case firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_OUTPUT_RESTRICTED:
    {
        return firebolt::rialto::KeyStatus::OUTPUT_RESTRICTED;
    }
    case firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_PENDING:
    {
        return firebolt::rialto::KeyStatus::PENDING;
    }
    case firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_INTERNAL_ERROR:
    {
        return firebolt::rialto::KeyStatus::INTERNAL_ERROR;
    }
    case firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_RELEASED:
    {
        return firebolt::rialto::KeyStatus::RELEASED;
    }
    }
    return firebolt::rialto::KeyStatus::INTERNAL_ERROR;
}
#endif // MEDIA_KEYS_PROTO_UTILS_H_
