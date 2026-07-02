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

#include "NeedDataDelayCalculator.h"

namespace
{
constexpr std::chrono::milliseconds kDefaultNeedMediaDataResendTimeMs{15};
constexpr std::chrono::milliseconds kMaxDelayMs{500};
} // namespace

namespace firebolt::rialto::server
{
std::chrono::milliseconds NeedDataDelayCalculator::getNeedMediaDataDelay(MediaSourceType mediaSourceType)
{
    auto it = m_delays.find(mediaSourceType);
    if (it == m_delays.end())
    {
        m_delays[mediaSourceType] = kDefaultNeedMediaDataResendTimeMs;
        return kDefaultNeedMediaDataResendTimeMs;
    }
    return it->second;
}

void NeedDataDelayCalculator::increaseNeedMediaDataDelay(MediaSourceType mediaSourceType)
{
    auto it = m_delays.find(mediaSourceType);
    if (it == m_delays.end())
    {
        m_delays[mediaSourceType] = kDefaultNeedMediaDataResendTimeMs;
        return;
    }
    if (it->second * 2 < kMaxDelayMs)
    {
        it->second *= 2;
    }
}

void NeedDataDelayCalculator::decreaseNeedMediaDataDelay(MediaSourceType mediaSourceType)
{
    auto it = m_delays.find(mediaSourceType);
    if (it != m_delays.end() && it->second / 2 >= kDefaultNeedMediaDataResendTimeMs)
    {
        it->second /= 2;
    }
}

void NeedDataDelayCalculator::resetMediaDataDelay(MediaSourceType mediaSourceType)
{
    m_delays[mediaSourceType] = kDefaultNeedMediaDataResendTimeMs;
}

void NeedDataDelayCalculator::resetMediaDataDelay()
{
    for (auto &delay : m_delays)
    {
        delay.second = kDefaultNeedMediaDataResendTimeMs;
    }
}
} // namespace firebolt::rialto::server
