#include "RialtoMetrics.h"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <gst/gst.h>
#include <thread>
#include <vector>

using namespace firebolt::rialto::server;

TEST(RialtoMetricsTest, ParsesElementMetricMessageAndUpdatesFrameCounters)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric_type", G_TYPE_UINT, 0U, "phase",
                                                 G_TYPE_UINT, 0U, "source_id", G_TYPE_UINT, 11U, "session_id",
                                                 G_TYPE_UINT, 7U, "pts_ns", G_TYPE_INT64, gint64{9001}, nullptr);
    ASSERT_NE(nullptr, structure);
    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    gst_object_unref(source);
    ASSERT_NE(nullptr, message);

    RialtoMetrics metrics;
    metrics.setSessionId(7);
    const auto beforeParse = std::chrono::steady_clock::now();
    const auto parsed = metrics.onMetricMessage(message, std::uint64_t{101}, std::uint64_t{7});
    const auto repeated = metrics.onMetricMessage(message, std::uint64_t{101}, std::uint64_t{7});
    const auto withoutStats = metrics.onMetricMessage(message, std::nullopt, std::nullopt);
    const auto afterParse = std::chrono::steady_clock::now();
    gst_message_unref(message);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(7, parsed->sessionId);
    EXPECT_EQ(0U, parsed->type);
    EXPECT_EQ(0U, parsed->phase);
    EXPECT_EQ(11, parsed->sourceId);
    EXPECT_EQ(9001U, parsed->ptsNs);
    EXPECT_EQ(7U, parsed->counters.dropped);
    ASSERT_TRUE(repeated.has_value());
    EXPECT_EQ(7U, repeated->counters.dropped);
    ASSERT_TRUE(withoutStats.has_value());
    EXPECT_EQ(101U, withoutStats->counters.rendered);
    EXPECT_EQ(7U, withoutStats->counters.dropped);
    FrameCounters counters;
    ASSERT_TRUE(metrics.getFrameCounters(7, 11, counters));
    EXPECT_EQ(101U, counters.rendered);
    EXPECT_EQ(7U, counters.dropped);
    EXPECT_EQ(0U, counters.corrupted);
    EXPECT_GE(counters.timestamp, static_cast<std::uint64_t>(
                                      std::chrono::duration_cast<std::chrono::nanoseconds>(beforeParse.time_since_epoch())
                                          .count()));
    EXPECT_LE(counters.timestamp, static_cast<std::uint64_t>(
                                      std::chrono::duration_cast<std::chrono::nanoseconds>(afterParse.time_since_epoch())
                                          .count()));
    EXPECT_FALSE(metrics.getFrameCounters(8, 11, counters));
}

TEST(RialtoMetricsTest, IgnoresMalformedElementMetricMessage)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric_type", G_TYPE_UINT, 0U, nullptr);
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

TEST(RialtoMetricsTest, CountsDecodeErrorsAndIgnoresEpisodeMessages)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    RialtoMetrics metrics;
    metrics.setSessionId(7);

    auto postMetric = [&](guint type, guint phase, guint64 ptsNs)
    {
        GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric_type", G_TYPE_UINT, type, "phase",
                                                     G_TYPE_UINT, phase, "source_id", G_TYPE_UINT, 11U, "session_id",
                                                     G_TYPE_UINT, 7U, "pts_ns", G_TYPE_INT64,
                                                     static_cast<gint64>(ptsNs), nullptr);
        ASSERT_NE(nullptr, structure);
        GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
        ASSERT_NE(nullptr, message);
        const auto parsed = metrics.onMetricMessage(message, std::nullopt, std::nullopt);
        gst_message_unref(message);
        ASSERT_TRUE(parsed.has_value());
        EXPECT_EQ(ptsNs, parsed->ptsNs);
    };

    postMetric(0U, 0U, 9000U);
    postMetric(2U, 0U, 9100U);
    postMetric(2U, 1U, 9200U);

    gst_object_unref(source);
    FrameCounters counters;
    ASSERT_TRUE(metrics.getFrameCounters(7, 11, counters));
    EXPECT_EQ(0U, counters.dropped);
    EXPECT_EQ(1U, counters.corrupted);
    EXPECT_GT(counters.timestamp, 0U);
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