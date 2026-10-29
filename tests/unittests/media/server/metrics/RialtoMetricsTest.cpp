#include "RialtoMetrics.h"
#include <atomic>
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
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric_type", G_TYPE_UINT, 0U, "source_id",
                                                 G_TYPE_UINT, 11U, "rendered_frames", G_TYPE_UINT64, guint64{101},
                                                 "dropped_frames", G_TYPE_UINT64, guint64{7}, "corrupted_frames",
                                                 G_TYPE_UINT64, guint64{3}, "pts_ns", G_TYPE_UINT64, guint64{9001},
                                                 nullptr);
    ASSERT_NE(nullptr, structure);
    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    gst_object_unref(source);
    ASSERT_NE(nullptr, message);

    RialtoMetrics metrics;
    metrics.setSessionId(7);
    const auto parsed = metrics.onMetricMessage(message);
    gst_message_unref(message);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(0U, parsed->type);
    EXPECT_EQ(11, parsed->sourceId);
    FrameCounters counters;
    ASSERT_TRUE(metrics.getFrameCounters(7, 11, counters));
    EXPECT_EQ(101U, counters.rendered);
    EXPECT_EQ(7U, counters.dropped);
    EXPECT_EQ(3U, counters.corrupted);
    EXPECT_EQ(9001U, counters.timestamp);
    EXPECT_FALSE(metrics.getFrameCounters(8, 11, counters));
}

TEST(RialtoMetricsTest, IgnoresMalformedElementMetricMessage)
{
    gst_init(nullptr, nullptr);

    GstElement *source = gst_pipeline_new("metrics-source");
    ASSERT_NE(nullptr, source);
    GstStructure *structure = gst_structure_new("media-pipeline-metric", "metric_type", G_TYPE_UINT, 0U, "source_id",
                                                 G_TYPE_UINT, 11U, nullptr);
    ASSERT_NE(nullptr, structure);
    GstMessage *message = gst_message_new_element(GST_OBJECT(source), structure);
    gst_object_unref(source);
    ASSERT_NE(nullptr, message);

    RialtoMetrics metrics;
    metrics.setSessionId(7);
    const auto parsed = metrics.onMetricMessage(message);
    gst_message_unref(message);

    EXPECT_FALSE(parsed.has_value());
    FrameCounters counters;
    EXPECT_FALSE(metrics.getFrameCounters(7, 11, counters));
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