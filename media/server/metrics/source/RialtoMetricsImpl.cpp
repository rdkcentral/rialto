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
} // namespace firebolt::rialto::server