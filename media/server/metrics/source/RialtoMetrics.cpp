#include "RialtoMetrics.h"
#include "RialtoLogging.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <gst/gst.h>
#include <limits>
#include <unordered_map>
#include <mutex>

namespace firebolt::rialto::server
{
enum MetricPhase : guint
{
	OCCURRENCE = 0,
	STARTED = 1,
	RESOLVED = 2,
	INVALID = 3
};

class ElementToSourceMapperImpl : public ElementToSourceMapper
{
private:
	std::unordered_map<uintptr_t, std::pair<uint32_t, uint32_t>> m_elementMap;
	mutable std::mutex m_mapMutex;

public:
	ElementToSourceMapperImpl() : m_elementMap(), m_mapMutex()
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ElementToSourceMapperImpl created");
	}

	virtual ~ElementToSourceMapperImpl()
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ElementToSourceMapperImpl destroyed");
	}

	void RegisterElement(GstElement* element, uint32_t session_id, uint32_t source_id) override
	{
		if (!element)
		{
			RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, "ElementToSourceMapperImpl::RegisterElement: null element provided");
			return;
		}

		std::lock_guard<std::mutex> lock(m_mapMutex);
		uintptr_t elementKey = reinterpret_cast<uintptr_t>(element);
		m_elementMap[elementKey] = std::make_pair(session_id, source_id);
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, 
						"ElementToSourceMapperImpl::RegisterElement: registered element %p with session_id %u, source_id %u",
						element, session_id, source_id);
	}

	bool MapElement(GstElement* element, uint32_t& out_session_id, uint32_t& out_source_id) override
	{
		if (!element)
		{
			RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, "ElementToSourceMapperImpl::MapElement: null element provided");
			return false;
		}

		std::lock_guard<std::mutex> lock(m_mapMutex);
		uintptr_t elementKey = reinterpret_cast<uintptr_t>(element);
		auto it = m_elementMap.find(elementKey);
		
		if (it == m_elementMap.end())
		{
			RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, 
							"ElementToSourceMapperImpl::MapElement: element %p not found in map", element);
			return false;
		}

		out_session_id = it->second.first;
		out_source_id = it->second.second;
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
						"ElementToSourceMapperImpl::MapElement: mapped element %p to session_id %u, source_id %u",
						element, out_session_id, out_source_id);
		return true;
	}

	void UnregisterElement(GstElement* element) override
	{
		if (!element)
		{
			return;
		}

		std::lock_guard<std::mutex> lock(m_mapMutex);
		if (m_elementMap.erase(reinterpret_cast<uintptr_t>(element)) > 0)
		{
			RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ElementToSourceMapperImpl::UnregisterElement: removed element %p", element);
		}
	}

	void UnregisterAllElements() override
	{
		std::lock_guard<std::mutex> lock(m_mapMutex);
		m_elementMap.clear();
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ElementToSourceMapperImpl::UnregisterAllElements: map cleared");
	}
};

namespace
{
std::uint64_t getMonotonicTimestampNs()
{
	return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
	                                      std::chrono::steady_clock::now().time_since_epoch())
	                                      .count());
}

std::int64_t getUnixTimestampNs()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
	    .count();
}
} // namespace

const char *metricTypeToString(std::uint32_t type)
{
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "metricTypeToString requested for type: %u",
					 static_cast<unsigned int>(type));
	switch (type)
	{
	case 1:
		return "METRIC_VIDEO_FRAME_DROP";
	case 2:
		return "METRIC_AUDIO_FRAME_DROP";
	case 3:
		return "METRIC_VIDEO_DECODE_ERROR";
	case 4:
		return "METRIC_AUDIO_DECODE_ERROR";
	case 5:
		return "METRIC_VIDEO_FRAME_REPEAT";
	case 6:
		return "METRIC_AUDIO_GAP";
	case 7:
		return "METRIC_BUFFER_UNDERFLOW";
	default:
		return "METRIC_UNKNOWN";
	}
}

RialtoMetrics::RialtoMetrics() : m_elementToSourceMapper(std::make_unique<ElementToSourceMapperImpl>())
{
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics created");
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
	if (!structure)
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics::onMetricMessage ignored message without structure");
		return std::nullopt;
	}

	if (!gst_structure_has_name(structure, "media-pipeline-metric"))
	{
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "RialtoMetrics::onMetricMessage ignored non-media-pipeline-metric message");
		return std::nullopt;
	}

	// Validate the metric message using the new ValidateMediaPipelineMetric function
	ParsedMetricMessage parsedMessage;
	GstFlowReturn validationResult = ValidateMediaPipelineMetric(structure, parsedMessage, message, m_elementToSourceMapper.get());
	
	if (validationResult != GST_FLOW_OK)
	{
		RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, "RialtoMetrics::onMetricMessage validation failed for metric message");
		return std::nullopt;
	}

	// The GstMessage timestamp is taken from a monotonic clock when the message was posted on the bus.
	const std::uint64_t nowMonotonicNs = getMonotonicTimestampNs();
	const std::int64_t nowUnixNs = getUnixTimestampNs();
	const GstClockTime messageTimestamp = GST_MESSAGE_TIMESTAMP(message);
	if (GST_CLOCK_TIME_IS_VALID(messageTimestamp) && messageTimestamp <= nowMonotonicNs)
	{
		parsedMessage.observed_at_monotonic_ns = messageTimestamp;
		parsedMessage.observed_at_unix_ns = nowUnixNs - static_cast<std::int64_t>(nowMonotonicNs - messageTimestamp);
	}
	else
	{
		parsedMessage.observed_at_monotonic_ns = nowMonotonicNs;
		parsedMessage.observed_at_unix_ns = nowUnixNs;
	}

	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
					 "RialtoMetrics::onMetricMessage validated metric: %u, observation_kind: %u, session_id: %d, source_id: %d",
					 parsedMessage.metric, parsedMessage.observation_kind, parsedMessage.session_id, parsedMessage.source_id);
	
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

ElementToSourceMapper* RialtoMetrics::getElementToSourceMapper()
{
	return m_elementToSourceMapper.get();
}

GstFlowReturn ValidateMediaPipelineMetric(const GstStructure* msg, ParsedMetricMessage& out)
{
	if (!msg)
	{
		RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, "ValidateMediaPipelineMetric: null structure provided");
		return GST_FLOW_ERROR;
	}

	const char* structureName = gst_structure_get_name(msg);
	if (!structureName || strcmp(structureName, "media-pipeline-metric") != 0)
	{
		RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER,
						"ValidateMediaPipelineMetric: invalid structure name '%s', expected 'media-pipeline-metric'",
						structureName ? structureName : "null");
		return GST_FLOW_ERROR;
	}

	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ValidateMediaPipelineMetric: structure name validation passed");

	// Validate 'metric' field - required, must be G_TYPE_UINT, value in range 1-7
	if (!gst_structure_has_field(msg, "metric"))
	{
		RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, "ValidateMediaPipelineMetric: required field 'metric' is missing");
		return GST_FLOW_ERROR;
	}

	const GValue* metricValue = gst_structure_get_value(msg, "metric");
	if (!metricValue || !G_VALUE_HOLDS_UINT(metricValue))
	{
		RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, 
						"ValidateMediaPipelineMetric: field 'metric' has wrong type, expected G_TYPE_UINT, actual %s",
						metricValue ? G_VALUE_TYPE_NAME(metricValue) : "null");
		return GST_FLOW_ERROR;
	}

	guint metricVal = g_value_get_uint(metricValue);
	if (metricVal < static_cast<guint>(MediaMetricType::MEDIA_METRIC_VIDEO_FRAME_DROP) || 
	    metricVal > static_cast<guint>(MediaMetricType::MEDIA_METRIC_BUFFER_UNDERFLOW))
	{
		RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER,
						"ValidateMediaPipelineMetric: field 'metric' value %u is outside valid range %u-%u",
						metricVal,
						static_cast<guint>(MediaMetricType::MEDIA_METRIC_VIDEO_FRAME_DROP),
						static_cast<guint>(MediaMetricType::MEDIA_METRIC_BUFFER_UNDERFLOW));
		return GST_FLOW_ERROR;
	}

	out.metric = metricVal;
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ValidateMediaPipelineMetric: metric field validation passed with value %u", metricVal);

	// Validate 'observation-kind' field - required, must be G_TYPE_UINT, value in {1, 2, 3}
	if (!gst_structure_has_field(msg, "observation-kind"))
	{
		RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, "ValidateMediaPipelineMetric: required field 'observation-kind' is missing");
		return GST_FLOW_ERROR;
	}

	const GValue* observationKindValue = gst_structure_get_value(msg, "observation-kind");
	if (!observationKindValue || !G_VALUE_HOLDS_UINT(observationKindValue))
	{
		RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, 
						"ValidateMediaPipelineMetric: field 'observation-kind' has wrong type, expected G_TYPE_UINT, actual %s",
						observationKindValue ? G_VALUE_TYPE_NAME(observationKindValue) : "null");
		return GST_FLOW_ERROR;
	}

	guint observationKindVal = g_value_get_uint(observationKindValue);
	if (observationKindVal < static_cast<guint>(ObservationKind::OCCURRENCE) || 
	    observationKindVal > static_cast<guint>(ObservationKind::EPISODE_RESOLVED))
	{
		RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER,
						"ValidateMediaPipelineMetric: field 'observation-kind' value %u is outside valid range %u-%u",
						observationKindVal,
						static_cast<guint>(ObservationKind::OCCURRENCE),
						static_cast<guint>(ObservationKind::EPISODE_RESOLVED));
		return GST_FLOW_ERROR;
	}

	out.observation_kind = observationKindVal;
	RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ValidateMediaPipelineMetric: observation-kind field validation passed with value %u", observationKindVal);

	// Validate 'pts-ns' field - optional, when present must be G_TYPE_INT64
	if (gst_structure_has_field(msg, "pts-ns"))
	{
		const GValue* ptsNsValue = gst_structure_get_value(msg, "pts-ns");
		if (!ptsNsValue || !G_VALUE_HOLDS_INT64(ptsNsValue))
		{
			RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, 
							"ValidateMediaPipelineMetric: field 'pts-ns' has wrong type, expected G_TYPE_INT64, actual %s",
							ptsNsValue ? G_VALUE_TYPE_NAME(ptsNsValue) : "null");
			return GST_FLOW_ERROR;
		}

		gint64 ptsNsVal = g_value_get_int64(ptsNsValue);
		out.pts_ns = ptsNsVal;
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ValidateMediaPipelineMetric: pts-ns field validation passed with value %ld", ptsNsVal);
	}

	// Validate 'duration-ns' field - optional, when present must be G_TYPE_UINT64 and non-negative
	if (gst_structure_has_field(msg, "duration-ns"))
	{
		const GValue* durationNsValue = gst_structure_get_value(msg, "duration-ns");
		if (!durationNsValue || !G_VALUE_HOLDS_UINT64(durationNsValue))
		{
			RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, 
							"ValidateMediaPipelineMetric: field 'duration-ns' has wrong type, expected G_TYPE_UINT64, actual %s",
							durationNsValue ? G_VALUE_TYPE_NAME(durationNsValue) : "null");
			return GST_FLOW_ERROR;
		}

		guint64 durationNsVal = g_value_get_uint64(durationNsValue);
		out.duration_ns = durationNsVal;
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ValidateMediaPipelineMetric: duration-ns field validation passed with value %llu", 
						 static_cast<unsigned long long>(durationNsVal));
	}

	// Validate 'count' field - optional, when present must be G_TYPE_UINT64
	if (gst_structure_has_field(msg, "count"))
	{
		const GValue* countValue = gst_structure_get_value(msg, "count");
		if (!countValue || !G_VALUE_HOLDS_UINT64(countValue))
		{
			RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, 
							"ValidateMediaPipelineMetric: field 'count' has wrong type, expected G_TYPE_UINT64, actual %s",
							countValue ? G_VALUE_TYPE_NAME(countValue) : "null");
			return GST_FLOW_ERROR;
		}

		guint64 countVal = g_value_get_uint64(countValue);
		out.count = countVal;
		RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER, "ValidateMediaPipelineMetric: count field validation passed with value %llu", 
						 static_cast<unsigned long long>(countVal));
	}

	// Legacy field-name rejection - reject messages with old field names
	const char* legacyFieldNames[] = {"metric_type", "phase", "pts_ns"};
	for (const auto& legacyName : legacyFieldNames)
	{
		if (gst_structure_has_field(msg, legacyName))
		{
			RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER, 
							"ValidateMediaPipelineMetric: legacy field name '%s' found in message, rejecting", legacyName);
			return GST_FLOW_ERROR;
		}
	}

	// Semantic field-combination validation
	// Reject OCCURRENCE (observation-kind=1) with duration-ns or count present
	if (observationKindVal == static_cast<guint>(ObservationKind::OCCURRENCE))
	{
		if (gst_structure_has_field(msg, "duration-ns"))
		{
			RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER,
							"ValidateMediaPipelineMetric: OCCURRENCE observation-kind cannot have 'duration-ns' field");
			return GST_FLOW_ERROR;
		}
		if (gst_structure_has_field(msg, "count"))
		{
			RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER,
							"ValidateMediaPipelineMetric: OCCURRENCE observation-kind cannot have 'count' field");
			return GST_FLOW_ERROR;
		}
	}

	// Reject BUFFER_UNDERFLOW (metric=7) in EPISODE_RESOLVED (observation-kind=3) with count present
	if (metricVal == static_cast<guint>(MediaMetricType::MEDIA_METRIC_BUFFER_UNDERFLOW) &&
		observationKindVal == static_cast<guint>(ObservationKind::EPISODE_RESOLVED))
	{
		if (gst_structure_has_field(msg, "count"))
		{
			RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER,
							"ValidateMediaPipelineMetric: BUFFER_UNDERFLOW metric with EPISODE_RESOLVED observation-kind cannot have 'count' field");
			return GST_FLOW_ERROR;
		}
	}

	// Enrichment: Extract source element from GstMessage and map to session_id and source_id
	if (gstMessage && mapper)
	{
		GstElement* sourceElement = GST_MESSAGE_SRC(gstMessage);
		if (sourceElement)
		{
			uint32_t enrichedSessionId = 0;
			uint32_t enrichedSourceId = 0;
			
			if (mapper->MapElement(sourceElement, enrichedSessionId, enrichedSourceId))
			{
				out.session_id = static_cast<int32_t>(enrichedSessionId);
				out.source_id = static_cast<int32_t>(enrichedSourceId);
				RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
								"ValidateMediaPipelineMetric: enriched message with session_id %d, source_id %d",
								out.session_id, out.source_id);
			}
			else
			{
				RIALTO_LOG_WARN(RIALTO_COMPONENT_SERVER,
								"ValidateMediaPipelineMetric: failed to map element %p to session/source IDs, continuing with defaults",
								sourceElement);
			}
		}
		else
		{
			RIALTO_LOG_DEBUG(RIALTO_COMPONENT_SERVER,
							"ValidateMediaPipelineMetric: no source element available in GstMessage");
		}
	}

	return GST_FLOW_OK;
}

} // namespace firebolt::rialto::server
