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

#ifndef METADATA_PROTO_UTILS_H_
#define METADATA_PROTO_UTILS_H_

#include "MediaCommon.h"
#include "metadata.pb.h"

firebolt::rialto::SegmentAlignment
convertSegmentAlignment(const firebolt::rialto::MediaSegmentMetadata_SegmentAlignment &segmentAlignment)
{
    switch (segmentAlignment)
    {
    case firebolt::rialto::MediaSegmentMetadata_SegmentAlignment_ALIGNMENT_NAL:
    {
        return firebolt::rialto::SegmentAlignment::NAL;
    }
    case firebolt::rialto::MediaSegmentMetadata_SegmentAlignment_ALIGNMENT_AU:
    {
        return firebolt::rialto::SegmentAlignment::AU;
    }
    case firebolt::rialto::MediaSegmentMetadata_SegmentAlignment_ALIGNMENT_UNDEFINED:
    default:
    {
        return firebolt::rialto::SegmentAlignment::UNDEFINED;
    }
    }
}

firebolt::rialto::MediaSegmentMetadata_CipherMode convertCipherMode(const firebolt::rialto::CipherMode &cipherMode)
{
    switch (cipherMode)
    {
    case firebolt::rialto::CipherMode::CENC:
    {
        return firebolt::rialto::MediaSegmentMetadata_CipherMode_CENC;
    }
    case firebolt::rialto::CipherMode::CBC1:
    {
        return firebolt::rialto::MediaSegmentMetadata_CipherMode_CBC1;
    }
    case firebolt::rialto::CipherMode::CENS:
    {
        return firebolt::rialto::MediaSegmentMetadata_CipherMode_CENS;
    }
    case firebolt::rialto::CipherMode::CBCS:
    {
        return firebolt::rialto::MediaSegmentMetadata_CipherMode_CBCS;
    }
    case firebolt::rialto::CipherMode::UNKNOWN:
    default:
    {
        return firebolt::rialto::MediaSegmentMetadata_CipherMode_UNKNOWN;
    }
    }
}
#endif // METADATA_PROTO_UTILS_H_
