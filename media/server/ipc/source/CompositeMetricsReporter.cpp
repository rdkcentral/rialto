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

#include "CompositeMetricsReporter.h"

namespace firebolt::rialto::server::ipc
{
void CompositeMetricsReporter::addReporter(std::unique_ptr<IMetricsReporter> reporter)
{
    if (reporter)
    {
        m_reporters.push_back(std::move(reporter));
    }
}

void CompositeMetricsReporter::reportPeriodicSample(const PeriodicMetricsReport &report)
{
    for (auto &reporter : m_reporters)
    {
        reporter->reportPeriodicSample(report);
    }
}

void CompositeMetricsReporter::reportStateTransition(const StateTransitionReport &report)
{
    for (auto &reporter : m_reporters)
    {
        reporter->reportStateTransition(report);
    }
}

void CompositeMetricsReporter::reportThresholdExceeded(const ThresholdAlert &alert)
{
    for (auto &reporter : m_reporters)
    {
        reporter->reportThresholdExceeded(alert);
    }
}
} // namespace firebolt::rialto::server::ipc
