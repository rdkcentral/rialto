/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include <gtest/gtest.h>

#include "WebAudioTestCommon.h"

namespace firebolt::rialto::server::testcommon
{

std::string getPcmFormat(bool isFloat, bool isSigned, int sampleSize, bool isBigEndian)
{
    std::string format;

    if (isFloat)
    {
        EXPECT_FALSE(isSigned); // Sanity check
        format += "F";
    }
    else if (isSigned)
    {
        format += "S";
    }
    else
    {
        format += "U";
    }

    format += std::to_string(sampleSize);

    if (isBigEndian)
    {
        EXPECT_FALSE(isFloat); // Sanity check
        format += "BE";
    }
    else
    {
        format += "LE";
    }

    return format;
}

} // namespace firebolt::rialto::server::testcommon
