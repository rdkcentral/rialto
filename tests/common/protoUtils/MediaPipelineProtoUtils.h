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

#include "mediapipelinemodule.pb.h"
#include "MediaCommon.h"

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

#endif // MEDIA_PIPELINE_PROTO_UTILS_H_
