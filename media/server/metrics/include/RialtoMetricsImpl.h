#ifndef FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_IMPL_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_IMPL_H_

#include "RialtoMetrics.h"

namespace firebolt::rialto::server
{
class RialtoMetricsImpl final : public RialtoMetrics
{
public:
    ~RialtoMetricsImpl() override = default;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_IMPL_H_