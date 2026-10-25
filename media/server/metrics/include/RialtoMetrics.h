#ifndef FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_
#define FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_

typedef struct _GstMessage GstMessage;

namespace firebolt::rialto::server
{
class RialtoMetrics
{
public:
    virtual ~RialtoMetrics() = default;
    virtual void onMetricMessage(GstMessage *message) = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_METRICS_RIALTO_METRICS_H_