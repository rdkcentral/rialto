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

#ifndef FIREBOLT_RIALTO_SERVER_MATCHERS_H_
#define FIREBOLT_RIALTO_SERVER_MATCHERS_H_

#include "GenericPlayerContext.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace firebolt::rialto::server
{
bool operator==(const Rectangle &lhs, const Rectangle &rhs);
} // namespace firebolt::rialto::server

namespace firebolt::rialto
{
bool operator==(const Fraction &lhs, const Fraction &rhs);
} // namespace firebolt::rialto

MATCHER_P(CharStrMatcher, expectedStr, "")
{
    std::string actualStr = arg;
    return expectedStr == actualStr;
}

MATCHER(NotNullMatcher, "")
{
    return nullptr != arg;
}

MATCHER_P(arrayMatcher, vec, "")
{
    const uint8_t *kArray = static_cast<const uint8_t *>(arg);
    for (unsigned int i = 0; i < vec.size(); ++i)
    {
        if (vec[i] != kArray[i])
        {
            return false;
        }
    }
    return true;
}

#endif // FIREBOLT_RIALTO_SERVER_MATCHERS_H_
