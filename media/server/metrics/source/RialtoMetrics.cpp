#include "RialtoMetrics.h"
#include "RialtoLogging.h"
#include <algorithm>
#include <chrono>
#include <gst/gst.h>
#include <limits>

namespace firebolt::rialto::server
{
enum MetricPhase : guint
{
	OCCURRENCE = 0,
	STARTED = 1,
	RESOLVED = 2,
	INVALID = 3
};

namespace
{
std::uint64_t getMonotonicTimestampNs()
{
	return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
	                                      std::chrono::steady_clock::now().time_since_epoch())
	                                      .count());
}
} // namespace

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

std::optional<ParsedMetricMessage> RialtoMetrics::onMetricMessage(
	GstMessage *message, const std::optional<std::uint64_t> &renderedFrames,
	const std::optional<std::uint64_t> &droppedFrames)
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
	guint phase{0};
	guint sourceId{0};
	guint sessionId{0};
	gint64 ptsNs{0};
	if (!gst_structure_get_uint(structure, "metric_type", &metricType) ||
		!gst_structure_get_uint(structure, "phase", &phase) ||
		!gst_structure_get_uint(structure, "source_id", &sourceId) ||
		!gst_structure_get_uint(structure, "session_id", &sessionId) ||
		!gst_structure_get_int64(structure, "pts_ns", &ptsNs) ||
		sourceId > static_cast<guint>(std::numeric_limits<int32_t>::max()) ||
		sessionId > static_cast<guint>(std::numeric_limits<int32_t>::max()) || ptsNs < 0 ||
		phase >= static_cast<guint>(MetricPhase::INVALID))
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics::onMetricMessage ignored malformed metric message");
		return std::nullopt;
	}

	int32_t currentSessionId{-1};
	FrameCounters counters;
	std::uint64_t count{0};
	std::uint64_t durationNs{0};
	bool validSession{false};
	{
		std::lock_guard<std::mutex> lock{m_counterMutex};
		currentSessionId = m_sessionId;
		if (currentSessionId >= 0 && static_cast<int32_t>(sessionId) == currentSessionId)
		{
			const auto key = std::make_pair(currentSessionId, static_cast<int32_t>(sourceId));
			const auto it = m_counterCache.find(key);
			if (it != m_counterCache.end())
			{
				counters = it->second;
			}

			if (renderedFrames)
			{
				counters.rendered = std::max(counters.rendered, *renderedFrames);
			}
			if (droppedFrames)
			{
				counters.dropped = std::max(counters.dropped, *droppedFrames);
			}
			if (phase == static_cast<guint>(MetricPhase::OCCURRENCE) &&
				metricType == static_cast<guint>(firebolt::rialto::MetricType::VIDEO_DECODE_ERROR) &&
				counters.corrupted < std::numeric_limits<std::uint64_t>::max())
			{
				++counters.corrupted;
			}
			counters.timestamp = getMonotonicTimestampNs();
			m_counterCache[key] = counters;

			auto &episode = m_metricEpisodes[{currentSessionId, static_cast<int32_t>(sourceId), metricType}];
			if (phase == static_cast<guint>(MetricPhase::OCCURRENCE))
			{
				if (!episode.active)
				{
					episode.startPtsNs = static_cast<std::uint64_t>(ptsNs);
					episode.active = true;
					episode.count = 1;
					episode.durationNs = 0;
				}
				else
				{
					++episode.count;
					episode.durationNs = 0;
				}
			}
			else
			{
				++episode.count;
				if (episode.active && ptsNs >= static_cast<gint64>(episode.startPtsNs))
				{
					episode.durationNs = static_cast<std::uint64_t>(ptsNs) - episode.startPtsNs;
				}
				episode.active = false;
			}
			count = episode.count;
			durationNs = episode.durationNs;
			validSession = true;
		}
	}
	if (!validSession)
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
						 "RialtoMetrics::onMetricMessage ignored metric for sessionId: %u, current sessionId: %d",
						 sessionId, currentSessionId);
		return std::nullopt;
	}

	ParsedMetricMessage parsedMessage{static_cast<int32_t>(sessionId), metricType, phase,
							  static_cast<int32_t>(sourceId), static_cast<std::uint64_t>(ptsNs), counters,
							  count, durationNs};
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
					 "RialtoMetrics::onMetricMessage parsed sessionId: %d, sourceId: %d, type: %u, phase: %u, rendered: %llu, dropped: %llu, corrupted: %llu, ptsNs: %llu, count: %llu, durationNs: %llu",
					 parsedMessage.sessionId, parsedMessage.sourceId, metricType, phase,
					 static_cast<unsigned long long>(counters.rendered),
					 static_cast<unsigned long long>(counters.dropped),
					 static_cast<unsigned long long>(counters.corrupted),
					 static_cast<unsigned long long>(parsedMessage.ptsNs),
					 static_cast<unsigned long long>(parsedMessage.count),
					 static_cast<unsigned long long>(parsedMessage.durationNs));
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
	auto &current = m_counterCache[{sessionId, sourceId}];
	current.rendered = std::max(current.rendered, counters.rendered);
	current.dropped = std::max(current.dropped, counters.dropped);
	current.corrupted = std::max(current.corrupted, counters.corrupted);
	current.timestamp = counters.timestamp;
}

void RialtoMetrics::updateSinkStats(int32_t sessionId, int32_t sourceId, std::uint64_t renderedFrames,
								   std::uint64_t droppedFrames)
{
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
					 "RialtoMetrics::updateSinkStats sessionId: %d, sourceId: %d, rendered: %llu, dropped: %llu",
					 sessionId, sourceId, static_cast<unsigned long long>(renderedFrames),
					 static_cast<unsigned long long>(droppedFrames));
	std::lock_guard<std::mutex> lock{m_counterMutex};
	auto &counters = m_counterCache[{sessionId, sourceId}];
	counters.rendered = std::max(counters.rendered, renderedFrames);
	counters.dropped = std::max(counters.dropped, droppedFrames);
	counters.timestamp = getMonotonicTimestampNs();
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
