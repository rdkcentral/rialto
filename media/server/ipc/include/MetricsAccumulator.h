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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_METRICS_ACCUMULATOR_H_
#define FIREBOLT_RIALTO_SERVER_IPC_METRICS_ACCUMULATOR_H_

#include <cmath>
#include <cstdint>
#include <limits>

namespace firebolt::rialto::server::ipc
{
struct MetricsStatistics
{
    double min{0.0};
    double max{0.0};
    double mean{0.0};
    double stddev{0.0};
    std::uint64_t count{0};
};

/**
 * @brief Numerically stable running statistics using Welford's online algorithm.
 *        Computes min, max, mean, and standard deviation in O(1) memory.
 */
class MetricsAccumulator
{
public:
    MetricsAccumulator() = default;
    ~MetricsAccumulator() = default;

    void addSample(double value)
    {
        ++m_count;
        if (value < m_min)
        {
            m_min = value;
        }
        if (value > m_max)
        {
            m_max = value;
        }

        // Welford's online algorithm
        const double kDelta{value - m_mean};
        m_mean += kDelta / static_cast<double>(m_count);
        const double kDelta2{value - m_mean};
        m_m2 += kDelta * kDelta2;
    }

    void reset()
    {
        m_count = 0;
        m_min = std::numeric_limits<double>::max();
        m_max = std::numeric_limits<double>::lowest();
        m_mean = 0.0;
        m_m2 = 0.0;
    }

    MetricsStatistics getStats() const
    {
        MetricsStatistics stats;
        stats.count = m_count;
        if (m_count == 0)
        {
            return stats;
        }
        stats.min = m_min;
        stats.max = m_max;
        stats.mean = m_mean;
        stats.stddev = (m_count > 1) ? std::sqrt(m_m2 / static_cast<double>(m_count - 1)) : 0.0;
        return stats;
    }

    std::uint64_t getCount() const { return m_count; }

private:
    std::uint64_t m_count{0};
    double m_min{std::numeric_limits<double>::max()};
    double m_max{std::numeric_limits<double>::lowest()};
    double m_mean{0.0};
    double m_m2{0.0};
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_METRICS_ACCUMULATOR_H_
