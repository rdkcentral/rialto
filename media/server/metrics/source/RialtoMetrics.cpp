#include "RialtoMetrics.h"
#include "RialtoLogging.h"
#include <gst/gst.h>
#include <limits>

namespace firebolt::rialto::server
{
const char *metricTypeToString(std::uint32_t type)
{
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "metricTypeToString requested for type: %u",
					 static_cast<unsigned int>(type));
	switch (type)
	{
	case 0:
		return "METRIC_VIDEO_FRAME_DROP";
	case 1:
		return "METRIC_AUDIO_FRAME_DROP";
	case 2:
		return "METRIC_VIDEO_DECODE_ERROR";
	case 3:
		return "METRIC_AUDIO_DECODE_ERROR";
	case 4:
		return "METRIC_VIDEO_FRAME_REPEAT";
	case 5:
		return "METRIC_AUDIO_GAP";
	case 6:
		return "METRIC_BUFFER_UNDERFLOW";
	default:
		return "METRIC_UNKNOWN";
	}
}

RialtoMetrics::~RialtoMetrics()
{
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics destroyed");
}

void RialtoMetrics::setSessionId(int32_t sessionId)
{
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics::setSessionId sessionId: %d", sessionId);
	std::lock_guard<std::mutex> lock{m_counterMutex};
	m_sessionId = sessionId;
}

std::optional<ParsedMetricMessage> RialtoMetrics::onMetricMessage(GstMessage *message)
{
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics::onMetricMessage received message: %p", message);
	if (!message)
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics::onMetricMessage ignored null message");
		return std::nullopt;
	}

	const GstStructure *structure = gst_message_get_structure(message);
	if (!structure || !gst_structure_has_name(structure, "media-pipeline-metric"))
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics::onMetricMessage ignored non-metric message");
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
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics::onMetricMessage ignored malformed metric message");
		return std::nullopt;
	}

	int32_t sessionId;
	{
		std::lock_guard<std::mutex> lock{m_counterMutex};
		sessionId = m_sessionId;
	}
	if (sessionId < 0)
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
						 "RialtoMetrics::onMetricMessage ignored metric without a valid session");
		return std::nullopt;
	}

	ParsedMetricMessage parsedMessage{metricType, static_cast<int32_t>(sourceId),
									 FrameCounters{renderedFrames, droppedFrames, corruptedFrames, ptsNs}};
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
					 "RialtoMetrics::onMetricMessage parsed sessionId: %d, sourceId: %d, type: %u, rendered: %llu, dropped: %llu, corrupted: %llu, ptsNs: %llu",
					 sessionId, parsedMessage.sourceId, metricType, static_cast<unsigned long long>(renderedFrames),
					 static_cast<unsigned long long>(droppedFrames), static_cast<unsigned long long>(corruptedFrames),
					 static_cast<unsigned long long>(ptsNs));
	updateFrameCounters(sessionId, parsedMessage.sourceId, parsedMessage.counters);
	return parsedMessage;
}

void RialtoMetrics::updateFrameCounters(int32_t sessionId, int32_t sourceId, const FrameCounters &counters)
{
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
					 "RialtoMetrics::updateFrameCounters sessionId: %d, sourceId: %d, rendered: %llu, dropped: %llu, corrupted: %llu, timestamp: %llu",
					 sessionId, sourceId, static_cast<unsigned long long>(counters.rendered),
					 static_cast<unsigned long long>(counters.dropped),
					 static_cast<unsigned long long>(counters.corrupted),
					 static_cast<unsigned long long>(counters.timestamp));
	std::lock_guard<std::mutex> lock{m_counterMutex};
	m_counterCache[{sessionId, sourceId}] = counters;
}

bool RialtoMetrics::getFrameCounters(int32_t sessionId, int32_t sourceId, FrameCounters &counters) const
{
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
					 "RialtoMetrics::getFrameCounters sessionId: %d, sourceId: %d", sessionId, sourceId);
	bool found = false;
	{
		std::lock_guard<std::mutex> lock{m_counterMutex};
		const auto it = m_counterCache.find({sessionId, sourceId});
		if (it != m_counterCache.end())
		{
			counters = it->second;
			found = true;
		}
	}
	if (!found)
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics::getFrameCounters no snapshot found");
		return false;
	}

	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
					 "RialtoMetrics::getFrameCounters found rendered: %llu, dropped: %llu, corrupted: %llu, timestamp: %llu",
					 static_cast<unsigned long long>(counters.rendered),
					 static_cast<unsigned long long>(counters.dropped),
					 static_cast<unsigned long long>(counters.corrupted),
					 static_cast<unsigned long long>(counters.timestamp));
	return true;
}
} // namespace firebolt::rialto::server
