#include "RialtoMetrics.h"
#include "RialtoLogging.h"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <gst/gst.h>
#include <string>
#include <thread>
#include <vector>

using namespace firebolt::rialto::server;

TEST(RialtoMetricsTest, ParsesElementMetricMessageWithMappedSessionAndSource)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    RialtoMetrics metrics;
    metrics.getElementToSourceMapper()->RegisterElement(source, 7U, 11U);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U, "observation-kind",
                                                 G_TYPE_UINT, 1U, "pts-ns", G_TYPE_INT64, gint64{9001}, nullptr);
    ASSERT_NE(nullptr, structure);
    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    gst_object_unref(source);
    ASSERT_NE(nullptr, message);

    const auto parsed = metrics.onMetricMessage(message, std::nullopt, std::nullopt);
    gst_message_unref(message);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(7, parsed->session_id);
    EXPECT_EQ(11, parsed->source_id);
    EXPECT_EQ(1U, parsed->metric);
    EXPECT_EQ(1U, parsed->observation_kind);
    ASSERT_TRUE(parsed->pts_ns.has_value());
    EXPECT_EQ(9001, *parsed->pts_ns);
}

TEST(RialtoMetricsTest, UnmappedElementDefaultsSessionAndSourceToZero)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    RialtoMetrics metrics;

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U, "observation-kind",
                                                 G_TYPE_UINT, 1U, nullptr);
    ASSERT_NE(nullptr, structure);
    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    gst_object_unref(source);
    ASSERT_NE(nullptr, message);

    const auto parsed = metrics.onMetricMessage(message, std::nullopt, std::nullopt);
    gst_message_unref(message);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(0, parsed->session_id);
    EXPECT_EQ(0, parsed->source_id);
}

TEST(RialtoMetricsTest, ObservedAtUsesGstMessageTimestamp)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 3U, "observation-kind",
                                                 G_TYPE_UINT, 2U, nullptr);
    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    gst_object_unref(source);
    ASSERT_NE(nullptr, message);

    const auto nowNs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    constexpr std::uint64_t kAgeNs{1000000};
    GST_MESSAGE_TIMESTAMP(message) = nowNs - kAgeNs;

    RialtoMetrics metrics;
    metrics.setSessionId(7);
    const auto unixBefore = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();
    const auto parsed = metrics.onMetricMessage(message, std::nullopt, std::nullopt);
    gst_message_unref(message);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(nowNs - kAgeNs, parsed->observed_at_monotonic_ns);
    // The unix value is derived from the same instant, so it is at least the message age before the call.
    EXPECT_LE(parsed->observed_at_unix_ns, unixBefore - static_cast<std::int64_t>(kAgeNs));
}

TEST(RialtoMetricsTest, IgnoresMalformedElementMetricMessage)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    GstStructure *structure = gst_structure_new("media-pipeline-metric", nullptr);
    ASSERT_NE(nullptr, structure);
    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    gst_object_unref(source);
    ASSERT_NE(nullptr, message);

    RialtoMetrics metrics;
    metrics.setSessionId(7);
    const auto parsed = metrics.onMetricMessage(message, std::nullopt, std::nullopt);
    gst_message_unref(message);

    EXPECT_FALSE(parsed.has_value());
    FrameCounters counters;
    EXPECT_FALSE(metrics.getFrameCounters(7, 11, counters));
}

TEST(RialtoMetricsTest, AcceptsValidMessageAfterRejectedMessage)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    RialtoMetrics metrics;
    metrics.setSessionId(7);
    metrics.getElementToSourceMapper()->RegisterElement(source, 7U, 11U);

    auto postMetric = [&](guint metric, guint observationKind)
    {
        GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, metric,
                                                     "observation-kind", G_TYPE_UINT, observationKind, nullptr);
        GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
        const auto parsed = metrics.onMetricMessage(message, std::nullopt, std::nullopt);
        gst_message_unref(message);
        return parsed;
    };

    EXPECT_FALSE(postMetric(0U, 1U).has_value());
    const auto parsed = postMetric(1U, 1U);
    gst_object_unref(source);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(7, parsed->session_id);
    EXPECT_EQ(11, parsed->source_id);
}

TEST(RialtoMetricsTest, CountsVideoDecodeErrorOccurrencesAsCorrupted)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    RialtoMetrics metrics;
    metrics.setSessionId(7);
    metrics.getElementToSourceMapper()->RegisterElement(source, 7U, 11U);

    auto postMetric = [&](guint metric, guint observationKind)
    {
        GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, metric,
                                                     "observation-kind", G_TYPE_UINT, observationKind, nullptr);
        ASSERT_NE(nullptr, structure);
        GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
        ASSERT_NE(nullptr, message);
        const auto parsed = metrics.onMetricMessage(message, std::nullopt, std::nullopt);
        gst_message_unref(message);
        EXPECT_TRUE(parsed.has_value());
    };

    postMetric(3U, 1U);
    postMetric(3U, 1U);
    postMetric(4U, 1U);
    postMetric(1U, 1U);

    gst_object_unref(source);
    FrameCounters counters;
    ASSERT_TRUE(metrics.getFrameCounters(7, 11, counters));
    EXPECT_EQ(2U, counters.corrupted);
    EXPECT_EQ(0U, counters.dropped);
}


TEST(RialtoMetricsTest, ReturnsLatestSnapshotForSessionAndSource)
{
    RialtoMetrics metrics;
    metrics.updateFrameCounters(7, 11, FrameCounters{1, 2, 3, 4});
    metrics.updateFrameCounters(7, 11, FrameCounters{5, 6, 7, 8});

    FrameCounters counters;
    ASSERT_TRUE(metrics.getFrameCounters(7, 11, counters));
    EXPECT_EQ(5U, counters.rendered);
    EXPECT_EQ(6U, counters.dropped);
    EXPECT_EQ(7U, counters.corrupted);
    EXPECT_EQ(8U, counters.timestamp);
    EXPECT_FALSE(metrics.getFrameCounters(7, 12, counters));
}

TEST(RialtoMetricsTest, UpdatingSinkStatsUsesServerTimestampAndPreservesCorruption)
{
    RialtoMetrics metrics;
    metrics.updateFrameCounters(7, 11, FrameCounters{1, 2, 3, 4});
    metrics.updateSinkStats(7, 11, 9, 6);

    FrameCounters counters;
    ASSERT_TRUE(metrics.getFrameCounters(7, 11, counters));
    EXPECT_EQ(9U, counters.rendered);
    EXPECT_EQ(6U, counters.dropped);
    EXPECT_EQ(3U, counters.corrupted);
    EXPECT_GT(counters.timestamp, 0U);

    const std::uint64_t lastTimestamp{counters.timestamp};
    metrics.updateSinkStats(7, 11, 0, 0);
    ASSERT_TRUE(metrics.getFrameCounters(7, 11, counters));
    EXPECT_EQ(9U, counters.rendered);
    EXPECT_EQ(6U, counters.dropped);
    EXPECT_EQ(3U, counters.corrupted);
    EXPECT_GE(counters.timestamp, lastTimestamp);
}

TEST(RialtoMetricsTest, FrameCountersNeverDecrease)
{
    RialtoMetrics metrics;
    metrics.updateFrameCounters(7, 11, FrameCounters{10, 5, 3, 100});
    metrics.updateFrameCounters(7, 11, FrameCounters{8, 4, 2, 90});
    metrics.updateSinkStats(7, 11, 7, 3);

    FrameCounters counters;
    ASSERT_TRUE(metrics.getFrameCounters(7, 11, counters));
    EXPECT_EQ(10U, counters.rendered);
    EXPECT_EQ(5U, counters.dropped);
    EXPECT_EQ(3U, counters.corrupted);
}

TEST(RialtoMetricsTest, ConcurrentQueriesReadConsistentSnapshots)
{
    constexpr int32_t kSessionId{3};
    constexpr int32_t kSourceId{9};
    constexpr std::uint64_t kIterations{10000};
    constexpr unsigned int kReaderCount{4};

    RialtoMetrics metrics;
    metrics.updateFrameCounters(kSessionId, kSourceId, FrameCounters{0, 1, 2, 3});

    std::atomic<unsigned int> ready{0};
    std::atomic<bool> start{false};
    std::atomic<bool> snapshotsConsistent{true};
    std::vector<std::thread> readers;

    for (unsigned int reader = 0; reader < kReaderCount; ++reader)
    {
        readers.emplace_back([&]()
                             {
                                 ++ready;
                                 while (!start.load())
                                 {
                                     std::this_thread::yield();
                                 }

                                 for (std::uint64_t i = 0; i < kIterations; ++i)
                                 {
                                     FrameCounters counters;
                                     if (!metrics.getFrameCounters(kSessionId, kSourceId, counters) ||
                                         counters.dropped != counters.rendered + 1 ||
                                         counters.corrupted != counters.rendered + 2 ||
                                         counters.timestamp != counters.rendered + 3)
                                     {
                                         snapshotsConsistent = false;
                                         return;
                                     }
                                 }
                             });
    }

    std::thread writer([&]()
                       {
                           ++ready;
                           while (!start.load())
                           {
                               std::this_thread::yield();
                           }

                           for (std::uint64_t i = 1; i <= kIterations; ++i)
                           {
                               metrics.updateFrameCounters(kSessionId, kSourceId, FrameCounters{i, i + 1, i + 2, i + 3});
                           }
                       });

    while (ready.load() != kReaderCount + 1)
    {
        std::this_thread::yield();
    }
    start = true;

    writer.join();
    for (auto &reader : readers)
    {
        reader.join();
    }

    EXPECT_TRUE(snapshotsConsistent.load());
}

TEST(RialtoMetricsTest, RapidBusBurstOfValidAndInvalidMessagesLeavesStateConsistent)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    GstBus *bus = gst_bus_new();
    ASSERT_NE(nullptr, bus);

    RialtoMetrics metrics;
    metrics.setSessionId(7);
    metrics.getElementToSourceMapper()->RegisterElement(source, 7U, 11U);

    constexpr int kMessageCount = 200;
    for (int i = 0; i < kMessageCount; ++i)
    {
        GstStructure *structure = nullptr;
        if (i % 2 == 0)
        {
            structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 3U,
                                          "observation-kind", G_TYPE_UINT, 1U, nullptr);
        }
        else
        {
            structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 9U,
                                          "observation-kind", G_TYPE_UINT, 1U, nullptr);
        }
        ASSERT_NE(nullptr, structure);
        ASSERT_TRUE(gst_bus_post(bus, gst_message_new_element(GST_OBJECT(source), structure)));
    }

    int parsedCount = 0;
    int receivedCount = 0;
    while (GstMessage *message = gst_bus_timed_pop_filtered(bus, 0, GST_MESSAGE_ELEMENT))
    {
        const bool isValid = (receivedCount % 2 == 0);
        const auto parsed = metrics.onMetricMessage(message, std::nullopt, std::nullopt);
        gst_message_unref(message);
        EXPECT_EQ(isValid, parsed.has_value());
        if (parsed.has_value())
        {
            ++parsedCount;
        }
        ++receivedCount;
    }

    EXPECT_EQ(kMessageCount, receivedCount);
    EXPECT_EQ(kMessageCount / 2, parsedCount);

    FrameCounters counters;
    ASSERT_TRUE(metrics.getFrameCounters(7, 11, counters));
    EXPECT_EQ(static_cast<std::uint64_t>(kMessageCount / 2), counters.corrupted);
    EXPECT_EQ(0U, counters.dropped);

    gst_object_unref(bus);
    gst_object_unref(source);
}

TEST(ValidateMediaPipelineMetricTest, AcceptsValidMediaPipelineMetricStructureName)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_OK, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsStructureNamedMetric)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("metric", nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsStructureNamedLegacyMetric)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("legacy-metric", nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsNullStructure)
{
    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(nullptr, out);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsMissingMetricField)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsMetricFieldWithWrongType)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_INT64, gint64{5}, nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, AcceptsAllValidMetricValuesOneToSeven)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    for (guint metricVal = 1; metricVal <= 7; ++metricVal)
    {
        GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, metricVal, nullptr);
        ASSERT_NE(nullptr, structure);

        GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
        ASSERT_NE(nullptr, message);

        const GstStructure *msgStructure = gst_message_get_structure(message);
        ASSERT_NE(nullptr, msgStructure);

        ParsedMetricMessage out;
        GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

        gst_message_unref(message);

        EXPECT_EQ(GST_FLOW_OK, result) << "Failed for metric value " << metricVal;
        EXPECT_EQ(metricVal, out.metric) << "Metric value not correctly stored for value " << metricVal;
    }

    gst_object_unref(source);
}

TEST(ValidateMediaPipelineMetricTest, RejectsMetricValueZero)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 0U, nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsMetricValueEight)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 8U, nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsMetricValueTwoFiftFive)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 255U, nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsMissingObservationKindField)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U, nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsObservationKindFieldWithWrongType)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", 
                                                 "metric", G_TYPE_UINT, 1U,
                                                 "observation-kind", G_TYPE_INT64, gint64{1}, 
                                                 nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, AcceptsAllValidObservationKindValues)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    for (guint kindVal = 1; kindVal <= 3; ++kindVal)
    {
        GstStructure *structure = gst_structure_new("media-pipeline-metric", 
                                                     "metric", G_TYPE_UINT, 1U,
                                                     "observation-kind", G_TYPE_UINT, kindVal, 
                                                     nullptr);
        ASSERT_NE(nullptr, structure);

        GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
        ASSERT_NE(nullptr, message);

        const GstStructure *msgStructure = gst_message_get_structure(message);
        ASSERT_NE(nullptr, msgStructure);

        ParsedMetricMessage out;
        GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

        gst_message_unref(message);

        EXPECT_EQ(GST_FLOW_OK, result) << "Failed for observation-kind value " << kindVal;
        EXPECT_EQ(kindVal, out.observation_kind) << "Observation-kind value not correctly stored for value " << kindVal;
    }

    gst_object_unref(source);
}

TEST(ValidateMediaPipelineMetricTest, RejectsObservationKindValueZero)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", 
                                                 "metric", G_TYPE_UINT, 1U,
                                                 "observation-kind", G_TYPE_UINT, 0U, 
                                                 nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

TEST(ValidateMediaPipelineMetricTest, RejectsObservationKindValueFour)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", 
                                                 "metric", G_TYPE_UINT, 1U,
                                                 "observation-kind", G_TYPE_UINT, 4U, 
                                                 nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

namespace
{
std::string validateAndCaptureWarnings(GstStructure *structure, GstFlowReturn &result)
{
    std::string logs;
    setLogHandler(RIALTO_COMPONENT_SERVER,
                  [&logs](RIALTO_DEBUG_LEVEL level, const char *, int, const char *, const char *message,
                          std::size_t messageLen)
                  {
                      if (level == RIALTO_DEBUG_LEVEL_WARNING)
                      {
                          logs.append(message, messageLen);
                          logs.append("\n");
                      }
                  },
                  true);

    ParsedMetricMessage out;
    result = ValidateMediaPipelineMetric(structure, out);
    setLogHandler(RIALTO_COMPONENT_SERVER, nullptr, false);
    return logs;
}
void expectRejectedWithWarning(GstStructure *structure)
{
    ASSERT_NE(nullptr, structure);
    GstFlowReturn result{GST_FLOW_OK};
    const std::string logs = validateAndCaptureWarnings(structure, result);
    gst_structure_free(structure);
    EXPECT_EQ(GST_FLOW_ERROR, result);
    EXPECT_FALSE(logs.empty());
}
} // namespace

TEST(ValidateMediaPipelineMetricLoggingTest, EveryRejectionPathLogsWarning)
{
    gst_init(nullptr, nullptr);

    expectRejectedWithWarning(gst_structure_new("metric", nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "observation-kind", G_TYPE_UINT, 1U, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_STRING, "1",
                                                "observation-kind", G_TYPE_UINT, 1U, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 0U,
                                                "observation-kind", G_TYPE_UINT, 1U, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                "observation-kind", G_TYPE_STRING, "1", nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                "observation-kind", G_TYPE_UINT, 0U, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                "observation-kind", G_TYPE_UINT, 1U, "pts-ns", G_TYPE_UINT, 5U, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                "observation-kind", G_TYPE_UINT, 2U, "duration-ns", G_TYPE_INT64,
                                                gint64{5}, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                "observation-kind", G_TYPE_UINT, 2U, "count", G_TYPE_INT64,
                                                gint64{5}, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                "observation-kind", G_TYPE_UINT, 1U, "duration-ns", G_TYPE_UINT64,
                                                guint64{1}, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                "observation-kind", G_TYPE_UINT, 1U, "count", G_TYPE_UINT64,
                                                guint64{1}, nullptr));
    expectRejectedWithWarning(gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 7U,
                                                "observation-kind", G_TYPE_UINT, 3U, "count", G_TYPE_UINT64,
                                                guint64{1}, nullptr));
}

TEST(ValidateMediaPipelineMetricLoggingTest, WrongTypeLogsExpectedAndActualType)
{
    gst_init(nullptr, nullptr);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_STRING, "1", nullptr);
    ASSERT_NE(nullptr, structure);

    GstFlowReturn result{GST_FLOW_OK};
    const std::string logs = validateAndCaptureWarnings(structure, result);
    gst_structure_free(structure);

    EXPECT_EQ(GST_FLOW_ERROR, result);
    EXPECT_NE(std::string::npos, logs.find("field 'metric' has wrong type, expected G_TYPE_UINT, actual gchararray"));
}

TEST(ValidateMediaPipelineMetricLoggingTest, WrongPtsTypeLogsExpectedAndActualType)
{
    gst_init(nullptr, nullptr);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                 "observation-kind", G_TYPE_UINT, 1U, "pts-ns", G_TYPE_UINT, 5U,
                                                 nullptr);
    ASSERT_NE(nullptr, structure);

    GstFlowReturn result{GST_FLOW_OK};
    const std::string logs = validateAndCaptureWarnings(structure, result);
    gst_structure_free(structure);

    EXPECT_EQ(GST_FLOW_ERROR, result);
    EXPECT_NE(std::string::npos, logs.find("field 'pts-ns' has wrong type, expected G_TYPE_INT64, actual guint"));
}

TEST(ValidateMediaPipelineMetricLoggingTest, OutOfRangeMetricLogsRejectedValue)
{
    gst_init(nullptr, nullptr);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 9U,
                                                 "observation-kind", G_TYPE_UINT, 1U, nullptr);
    ASSERT_NE(nullptr, structure);

    GstFlowReturn result{GST_FLOW_OK};
    const std::string logs = validateAndCaptureWarnings(structure, result);
    gst_structure_free(structure);

    EXPECT_EQ(GST_FLOW_ERROR, result);
    EXPECT_NE(std::string::npos, logs.find("field 'metric' value 9 is outside valid range"));
}

TEST(ElementToSourceMapperTest, UnregisterElementRemovesOnlyThatElement)
{
    gst_init(nullptr, nullptr);

    GstElement *first = gst_pipeline_new("first-element");
    GstElement *second = gst_pipeline_new("second-element");
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);

    RialtoMetrics metrics;
    ElementToSourceMapper *mapper = metrics.getElementToSourceMapper();
    mapper->RegisterElement(first, 1U, 2U);
    mapper->RegisterElement(second, 3U, 4U);

    mapper->UnregisterElement(first);

    uint32_t sessionId{0};
    uint32_t sourceId{0};
    EXPECT_FALSE(mapper->MapElement(first, sessionId, sourceId));
    EXPECT_TRUE(mapper->MapElement(second, sessionId, sourceId));
    EXPECT_EQ(3U, sessionId);
    EXPECT_EQ(4U, sourceId);

    gst_object_unref(first);
    gst_object_unref(second);
}

TEST(ElementToSourceMapperTest, UnregisterAllElementsClearsEveryMapping)
{
    gst_init(nullptr, nullptr);

    GstElement *first = gst_pipeline_new("first-element");
    GstElement *second = gst_pipeline_new("second-element");
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);

    RialtoMetrics metrics;
    ElementToSourceMapper *mapper = metrics.getElementToSourceMapper();
    mapper->RegisterElement(first, 1U, 2U);
    mapper->RegisterElement(second, 3U, 4U);

    mapper->UnregisterAllElements();

    uint32_t sessionId{0};
    uint32_t sourceId{0};
    EXPECT_FALSE(mapper->MapElement(first, sessionId, sourceId));
    EXPECT_FALSE(mapper->MapElement(second, sessionId, sourceId));

    gst_object_unref(first);
    gst_object_unref(second);
}

TEST(ValidateMediaPipelineMetricTest, RejectsCountValueZero)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 7U,
                                                 "observation-kind", G_TYPE_UINT, 2U,
                                                 "count", G_TYPE_UINT64, guint64{0}, nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}

static GstFlowReturn validateOwnedStructure(GstStructure *structure, ParsedMetricMessage &out)
{
    gst_init(nullptr, nullptr);
    GstElement *source = gst_pipeline_new("test-source");
    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    const GstStructure *msgStructure = gst_message_get_structure(message);
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);
    gst_message_unref(message);
    gst_object_unref(source);
    return result;
}

TEST(ValidateMediaPipelineMetricTest, AcceptsPtsNsAsInt64IncludingZeroAndNegative)
{
    for (gint64 pts : {gint64{0}, gint64{-1000}, gint64{1000000}})
    {
        GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                     "observation-kind", G_TYPE_UINT, 1U,
                                                     "pts-ns", G_TYPE_INT64, pts, nullptr);
        ParsedMetricMessage out;
        EXPECT_EQ(GST_FLOW_OK, validateOwnedStructure(structure, out));
        ASSERT_TRUE(out.pts_ns.has_value());
        EXPECT_EQ(pts, *out.pts_ns);
    }
}

TEST(ValidateMediaPipelineMetricTest, RejectsPtsNsWithWrongType)
{
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 1U,
                                                 "observation-kind", G_TYPE_UINT, 1U,
                                                 "pts-ns", G_TYPE_UINT64, guint64{5}, nullptr);
    ParsedMetricMessage out;
    EXPECT_EQ(GST_FLOW_ERROR, validateOwnedStructure(structure, out));
}

TEST(ValidateMediaPipelineMetricTest, AcceptsDurationNsAsUint64)
{
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 6U,
                                                 "observation-kind", G_TYPE_UINT, 3U,
                                                 "duration-ns", G_TYPE_UINT64, guint64{250000}, nullptr);
    ParsedMetricMessage out;
    EXPECT_EQ(GST_FLOW_OK, validateOwnedStructure(structure, out));
    ASSERT_TRUE(out.duration_ns.has_value());
    EXPECT_EQ(250000U, *out.duration_ns);
}

TEST(ValidateMediaPipelineMetricTest, RejectsDurationNsWithWrongType)
{
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 6U,
                                                 "observation-kind", G_TYPE_UINT, 3U,
                                                 "duration-ns", G_TYPE_INT64, gint64{250000}, nullptr);
    ParsedMetricMessage out;
    EXPECT_EQ(GST_FLOW_ERROR, validateOwnedStructure(structure, out));
}

TEST(ValidateMediaPipelineMetricTest, AcceptsCountValuesAboveOne)
{
    for (guint64 count : {guint64{5}, guint64{1000}})
    {
        GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 5U,
                                                     "observation-kind", G_TYPE_UINT, 3U,
                                                     "count", G_TYPE_UINT64, count, nullptr);
        ParsedMetricMessage out;
        EXPECT_EQ(GST_FLOW_OK, validateOwnedStructure(structure, out));
        ASSERT_TRUE(out.count.has_value());
        EXPECT_EQ(count, *out.count);
    }
}

TEST(ValidateMediaPipelineMetricTest, RejectsCountWithWrongType)
{
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 5U,
                                                 "observation-kind", G_TYPE_UINT, 3U,
                                                 "count", G_TYPE_UINT, 4U, nullptr);
    ParsedMetricMessage out;
    EXPECT_EQ(GST_FLOW_ERROR, validateOwnedStructure(structure, out));
}

TEST(ValidateMediaPipelineMetricTest, AcceptsEpisodeResolvedWithCountForVideoFrameRepeat)
{
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 5U,
                                                 "observation-kind", G_TYPE_UINT, 3U,
                                                 "count", G_TYPE_UINT64, guint64{4}, nullptr);
    ParsedMetricMessage out;
    EXPECT_EQ(GST_FLOW_OK, validateOwnedStructure(structure, out));
}

TEST(ValidateMediaPipelineMetricTest, AcceptsEpisodeResolvedWithDurationForAudioGap)
{
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 6U,
                                                 "observation-kind", G_TYPE_UINT, 3U,
                                                 "duration-ns", G_TYPE_UINT64, guint64{100}, nullptr);
    ParsedMetricMessage out;
    EXPECT_EQ(GST_FLOW_OK, validateOwnedStructure(structure, out));
}

TEST(ValidateMediaPipelineMetricTest, RejectsBufferUnderflowEpisodeResolvedWithCount)
{
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT, 7U,
                                                 "observation-kind", G_TYPE_UINT, 3U,
                                                 "count", G_TYPE_UINT64, guint64{2}, nullptr);
    ParsedMetricMessage out;
    EXPECT_EQ(GST_FLOW_ERROR, validateOwnedStructure(structure, out));
}

TEST(ValidateMediaPipelineMetricTest, RejectsObservationKindValueTwoFiftFive)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("test-source");
    ASSERT_NE(nullptr, source);

    GstStructure *structure = gst_structure_new("media-pipeline-metric", 
                                                 "metric", G_TYPE_UINT, 1U,
                                                 "observation-kind", G_TYPE_UINT, 255U, 
                                                 nullptr);
    ASSERT_NE(nullptr, structure);

    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    ASSERT_NE(nullptr, message);

    const GstStructure *msgStructure = gst_message_get_structure(message);
    ASSERT_NE(nullptr, msgStructure);

    ParsedMetricMessage out;
    GstFlowReturn result = ValidateMediaPipelineMetric(msgStructure, out);

    gst_message_unref(message);
    gst_object_unref(source);

    EXPECT_EQ(GST_FLOW_ERROR, result);
}