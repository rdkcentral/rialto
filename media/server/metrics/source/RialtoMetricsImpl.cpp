#include "RialtoMetricsImpl.h"
#include <gst/gst.h>

namespace firebolt::rialto::server
{
void RialtoMetricsImpl::onMetricMessage(GstMessage *message)
{
	if (!message)
	{
		return;
	}

	const GstStructure *structure = gst_message_get_structure(message);
	if (!structure || !gst_structure_has_name(structure, "media-pipeline-metric"))
	{
		return;
	}
}


void RialtoMetricsImpl::updateFrameCounters(int32_t sessionId, int32_t sourceId, const FrameCounters &counters)
{
	std::lock_guard<std::mutex> lock{m_counterMutex};
	m_counterCache[{sessionId, sourceId}] = counters;
}

bool RialtoMetricsImpl::getFrameCounters(int32_t sessionId, int32_t sourceId, FrameCounters &counters) const
{
	std::lock_guard<std::mutex> lock{m_counterMutex};
	const auto it = m_counterCache.find({sessionId, sourceId});
	if (it == m_counterCache.end())
	{
		return false;
	}

	counters = it->second;
	return true;
}
} // namespace firebolt::rialto::server