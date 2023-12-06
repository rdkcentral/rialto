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

#ifndef METADATA_PROTO_MATCHERS_H_
#define METADATA_PROTO_MATCHERS_H_

#include "MediaCommon.h"
#include "metadata.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

MATCHER_P(frameRateMatcher, expectedFrameRate, "")
{
    const firebolt::rialto::MediaSegmentMetadata_Fraction kFrameRate = arg;
    return ((kFrameRate.has_numerator()) && (kFrameRate.has_denominator()) &&
            (kFrameRate.numerator() == expectedFrameRate.numerator) &&
            (kFrameRate.denominator() == expectedFrameRate.denominator));
}
#endif // METADATA_PROTO_MATCHERS_H_
