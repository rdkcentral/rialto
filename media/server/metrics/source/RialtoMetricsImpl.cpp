#include "RialtoMetricsImpl.h"
#include <gst/gst.h>
#include <limits>

namespace firebolt::rialto::server
{
void RialtoMetricsImpl::setSessionId(int32_t sessionId)
{
	std::lock_guard<std::mutex> lock{m_counterMutex};
	m_sessionId = sessionId;
}

std::optional<ParsedMetricMessage> RialtoMetricsImpl::onMetricMessage(GstMessage *message)
{
	if (!message)
	{
		return std::nullopt;
	}

	const GstStructure *structure = gst_message_get_structure(message);
	if (!structure || !gst_structure_has_name(structure, "media-pipeline-metric"))
	{
		return std::nullopt;
	}

	guint metricType{0};
	guint sourceId{0};
	guint64 renderedFrames{0};
	guint64 droppedFrames{0};
	guint64 corruptedFrames{0};
	guint64 ptsNs{0};
	if (!gst_structure_get_uint(structure, "metric_type", &metricType) ||
		!gst_structure_get_uint(structure, "source_id", &sourceId) ||
		!gst_structure_get_uint64(structure, "rendered_frames", &renderedFrames) ||
		!gst_structure_get_uint64(structure, "dropped_frames", &droppedFrames) ||
		!gst_structure_get_uint64(structure, "corrupted_frames", &corruptedFrames) ||
		!gst_structure_get_uint64(structure, "pts_ns", &ptsNs) ||
		sourceId > static_cast<guint>(std::numeric_limits<int32_t>::max()))
	{
		return std::nullopt;
	}

	int32_t sessionId;
	{
		std::lock_guard<std::mutex> lock{m_counterMutex};
		sessionId = m_sessionId;
	}
	if (sessionId < 0)
	{
		return std::nullopt;
	}

	ParsedMetricMessage parsedMessage{metricType, static_cast<int32_t>(sourceId),
								   FrameCounters{renderedFrames, droppedFrames, corruptedFrames, ptsNs}};
	updateFrameCounters(sessionId, parsedMessage.sourceId, parsedMessage.counters);
	return parsedMessage;
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