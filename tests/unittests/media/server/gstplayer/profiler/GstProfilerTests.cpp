/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GlibWrapperMock.h"
#include "GstWrapperMock.h"

#include <cstdlib>
#include <glib.h>
#include <gst/gst.h>
#include <gtest/gtest.h>
#include <string_view>
#include <thread>

#define private public
#include "GstProfiler.h"
#undef private

using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;
using ::testing::StrEq;
using ::testing::StrictMock;

class GstProfilerTests : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock;
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock;
    std::unique_ptr<GstProfiler> m_gstProfiler;
    std::optional<std::string> m_profilerEnabledEnv;
    GstElement *m_realElement{nullptr};

    GstElement m_pipeline = {};
    GstElement m_element = {};
    GstPad m_pad = {};

    void SetUp() override
    {
        if (const char *env = std::getenv("PROFILER_ENABLED"))
            m_profilerEnabledEnv = env;

        m_gstWrapperMock = std::make_shared<StrictMock<GstWrapperMock>>();
        m_glibWrapperMock = std::make_shared<StrictMock<GlibWrapperMock>>();

        gst_init(nullptr, nullptr);
        m_realElement = gst_element_factory_make("fakesrc", "profiler-test-source");
        ASSERT_NE(m_realElement, nullptr);
    }

    void TearDown() override
    {
        if (m_realElement)
        {
            gst_object_unref(m_realElement);
            m_realElement = nullptr;
        }

        if (m_profilerEnabledEnv)
            setenv("PROFILER_ENABLED", m_profilerEnabledEnv->c_str(), 1);
        else
            unsetenv("PROFILER_ENABLED");
    }

    void setProfilerEnabledEnv(bool enabled) { setenv("PROFILER_ENABLED", enabled ? "true" : "false", 1); }

    void createGstProfiler(GstElement *pipeline = nullptr)
    {
        const char *profilerEnabledEnv = std::getenv("PROFILER_ENABLED");
        const bool expectPipelineRef = pipeline && profilerEnabledEnv && std::string_view{profilerEnabledEnv} == "true";
        if (expectPipelineRef)
        {
            EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(pipeline));
            EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(pipeline));
        }

        EXPECT_NO_THROW(m_gstProfiler = std::make_unique<GstProfiler>(pipeline, m_gstWrapperMock, m_glibWrapperMock));
        ASSERT_NE(m_gstProfiler, nullptr);
    }

    void expectElementRecognizedAsSource(GstElement *element)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(element)).WillOnce(Return(nullptr));

        EXPECT_CALL(*m_gstWrapperMock, gstElementClassGetMetadata(_, _)).WillOnce(Return("Source"));

        EXPECT_CALL(*m_glibWrapperMock, gStrrstr(_, _)).WillOnce(Return(const_cast<gchar *>("Source")));
    }

    void addRecordAndWait(const std::string &stage, const std::optional<std::string> &info = std::nullopt)
    {
        std::optional<IGstProfiler::RecordId> id;

        if (info)
            id = m_gstProfiler->createRecord(stage, *info);
        else
            id = m_gstProfiler->createRecord(stage);

        ASSERT_TRUE(id.has_value());
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
};

/**
 * Test that GstProfiler can be created with null pipeline.
 */
TEST_F(GstProfilerTests, CreateWithNullPipeline)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();
    EXPECT_TRUE(m_gstProfiler->isEnabled());
}

/**
 * Test that GstProfiler can be created with valid pipeline.
 */
TEST_F(GstProfilerTests, CreateWithPipeline)
{
    setProfilerEnabledEnv(true);
    createGstProfiler(&m_pipeline);
    EXPECT_TRUE(m_gstProfiler->isEnabled());
}

/**
 * Test that GstProfiler can create a record with stage only.
 */
TEST_F(GstProfilerTests, CreateRecordStageOnly)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    EXPECT_NO_THROW({ [[maybe_unused]] const auto id = m_gstProfiler->createRecord("PipelineCreated"); });
}

/**
 * Test that GstProfiler can create a record with stage and info.
 */
TEST_F(GstProfilerTests, CreateRecordStageAndInfo)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    EXPECT_NO_THROW({ [[maybe_unused]] const auto id = m_gstProfiler->createRecord("SourceAttached", "video"); });
}

/**
 * Test that GstProfiler can log a record.
 */
TEST_F(GstProfilerTests, LogRecord)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    EXPECT_NO_THROW(m_gstProfiler->logRecord(1));
}

/**
 * Test that GstProfiler can log pipeline metrics.
 */
TEST_F(GstProfilerTests, LogPipeline)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    EXPECT_NO_THROW(m_gstProfiler->logPipeline());
}

/**
 * Test that disabled profiler prevents record creation and scheduling.
 */
TEST_F(GstProfilerTests, DisabledProfilerPreventsRecordCreationAndScheduling)
{
    setProfilerEnabledEnv(false);
    createGstProfiler();

    EXPECT_FALSE(m_gstProfiler->isEnabled());
    EXPECT_FALSE(m_gstProfiler->createRecord("Pipeline Created").has_value());

    EXPECT_NO_THROW(m_gstProfiler->scheduleGstElementRecord(m_realElement));
}

/**
 * Test that enabled profiler allows scheduling path to proceed up to pad lookup.
 */
TEST_F(GstProfilerTests, EnabledProfilerAllowsScheduling)
{
    setProfilerEnabledEnv(true);
    EXPECT_NO_THROW(m_gstProfiler = std::make_unique<GstProfiler>(nullptr, m_gstWrapperMock, m_glibWrapperMock));
    ASSERT_NE(m_gstProfiler, nullptr);
    ASSERT_TRUE(m_gstProfiler->isEnabled());

    expectElementRecognizedAsSource(m_realElement);
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(m_realElement, StrEq("src"))).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(m_gstProfiler->scheduleGstElementRecord(m_realElement));
}

/**
 * Test that scheduling record for null element is safe.
 */
TEST_F(GstProfilerTests, ScheduleNullElementRecord)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    EXPECT_NO_THROW(m_gstProfiler->scheduleGstElementRecord(nullptr));
}

/**
 * Test that scheduling record for valid element is safe.
 */
TEST_F(GstProfilerTests, ScheduleElementRecord)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    auto *factory = reinterpret_cast<GstElementFactory *>(0x1);

    EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(m_realElement))
        .WillOnce(Return(nullptr))
        .WillOnce(Return(factory))
        .WillOnce(Return(factory));
    EXPECT_CALL(*m_gstWrapperMock, gstElementClassGetMetadata(_, _)).WillOnce(Return("Source"));
    EXPECT_CALL(*m_glibWrapperMock, gStrrstr(_, _)).WillOnce(Return(const_cast<gchar *>("Source")));

    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(m_realElement, StrEq("src"))).WillOnce(Return(&m_pad));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
        .WillOnce(Return(false));

    gchar *rawName = g_strdup("videoDecoder");
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetName(m_realElement)).WillOnce(Return(rawName));
    EXPECT_CALL(*m_glibWrapperMock, gFree(rawName)).WillOnce(Invoke([](gpointer ptr) { g_free(ptr); }));

    EXPECT_CALL(*m_gstWrapperMock, gstPadAddProbe(&m_pad, _, _, _, _))
        .WillOnce(Invoke(
            [](GstPad *pad, GstPadProbeType mask, GstPadProbeCallback callback, gpointer userData,
               GDestroyNotify destroyData) -> gulong
            {
                if (destroyData)
                {
                    destroyData(userData);
                }
                return 1;
            }));

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pad));

    EXPECT_NO_THROW(m_gstProfiler->scheduleGstElementRecord(m_realElement));
}

/**
 * Test that scheduled probe creates record with normalized video info.
 */
TEST_F(GstProfilerTests, ScheduleElementRecordCreatesProbeRecordWithVideoInfo)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    auto *factory = reinterpret_cast<GstElementFactory *>(0x1);

    EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(m_realElement)).WillOnce(Return(nullptr)).WillOnce(Return(factory));
    EXPECT_CALL(*m_gstWrapperMock, gstElementClassGetMetadata(_, _)).WillOnce(Return("Source"));
    EXPECT_CALL(*m_glibWrapperMock, gStrrstr(_, _)).WillOnce(Return(const_cast<gchar *>("Source")));

    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(m_realElement, StrEq("src"))).WillOnce(Return(&m_pad));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(true));

    EXPECT_CALL(*m_gstWrapperMock, gstPadAddProbe(&m_pad, _, _, _, _))
        .WillOnce(Invoke(
            [](GstPad *pad, GstPadProbeType mask, GstPadProbeCallback callback, gpointer userData,
               GDestroyNotify destroyData) -> gulong
            {
                GstBuffer *buffer = gst_buffer_new();
                GstPadProbeInfo info{};
                info.type = GST_PAD_PROBE_TYPE_BUFFER;
                info.data = buffer;

                EXPECT_EQ(callback(pad, &info, userData), GST_PAD_PROBE_REMOVE);

                gst_buffer_unref(buffer);
                if (destroyData)
                    destroyData(userData);

                return 1;
            }));

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pad));

    m_gstProfiler->scheduleGstElementRecord(m_realElement);

    const auto &records = m_gstProfiler->m_profiler->getRecords();
    ASSERT_FALSE(records.empty());
    EXPECT_EQ(records.back().stage, "Source FB Exit");
    EXPECT_EQ(records.back().info, "Video");
}

/**
 * Test that scheduled probe falls back to raw element name when media type cannot be classified.
 */
TEST_F(GstProfilerTests, ScheduleElementRecordFallsBackToRawElementName)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    auto *factory = reinterpret_cast<GstElementFactory *>(0x1);

    EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(m_realElement))
        .WillOnce(Return(nullptr))
        .WillOnce(Return(factory))
        .WillOnce(Return(factory));
    EXPECT_CALL(*m_gstWrapperMock, gstElementClassGetMetadata(_, _)).WillOnce(Return("Source"));
    EXPECT_CALL(*m_glibWrapperMock, gStrrstr(_, _)).WillOnce(Return(const_cast<gchar *>("Source")));

    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(m_realElement, StrEq("src"))).WillOnce(Return(&m_pad));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(_, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
        .WillOnce(Return(false));

    gchar *rawName = g_strdup("custom-element");
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetName(m_realElement)).WillOnce(Return(rawName));
    EXPECT_CALL(*m_glibWrapperMock, gFree(rawName)).WillOnce(Invoke([](gpointer ptr) { g_free(ptr); }));

    EXPECT_CALL(*m_gstWrapperMock, gstPadAddProbe(&m_pad, _, _, _, _))
        .WillOnce(Invoke(
            [](GstPad *pad, GstPadProbeType mask, GstPadProbeCallback callback, gpointer userData,
               GDestroyNotify destroyData) -> gulong
            {
                GstBuffer *buffer = gst_buffer_new();
                GstPadProbeInfo info{};
                info.type = GST_PAD_PROBE_TYPE_BUFFER;
                info.data = buffer;

                EXPECT_EQ(callback(pad, &info, userData), GST_PAD_PROBE_REMOVE);

                gst_buffer_unref(buffer);
                if (destroyData)
                    destroyData(userData);

                return 1;
            }));

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pad));

    m_gstProfiler->scheduleGstElementRecord(m_realElement);

    const auto &records = m_gstProfiler->m_profiler->getRecords();
    ASSERT_FALSE(records.empty());
    EXPECT_EQ(records.back().stage, "Source FB Exit");
    EXPECT_EQ(records.back().info, "custom-element");
}

/**
 * Test that scheduling record handles missing src pad.
 */
TEST_F(GstProfilerTests, ScheduleElementRecordNoPad)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    expectElementRecognizedAsSource(m_realElement);

    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(m_realElement, StrEq("src"))).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(m_gstProfiler->scheduleGstElementRecord(m_realElement));
}

/**
 * Test that GstProfiler can be destroyed after creation with pipeline.
 */
TEST_F(GstProfilerTests, DestroyAfterCreateWithPipeline)
{
    setProfilerEnabledEnv(true);
    createGstProfiler(&m_pipeline);

    EXPECT_NO_THROW(m_gstProfiler.reset());
}

/**
 * Test that element classes are mapped to expected stage names.
 */
TEST_F(GstProfilerTests, CheckElementMapsClassToExpectedStage)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(m_realElement)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementClassGetMetadata(_, _)).WillOnce(Return("Decryptor"));
    EXPECT_CALL(*m_glibWrapperMock, gStrrstr("Decryptor", StrEq("Source"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapperMock, gStrrstr("Decryptor", StrEq("Decryptor")))
        .WillOnce(Return(const_cast<gchar *>("Decryptor")));

    const auto stage = m_gstProfiler->checkElement(m_realElement);
    ASSERT_TRUE(stage.has_value());
    EXPECT_EQ(stage.value(), "Decryptor FB Exit");
}

/**
 * Test that metrics calculation returns null when required records are missing.
 */
TEST_F(GstProfilerTests, CalculateMetricsReturnsNulloptWhenRecordsMissing)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    addRecordAndWait("Pipeline Created");
    addRecordAndWait("All Sources Attached");

    EXPECT_FALSE(m_gstProfiler->calculateMetrics().has_value());
}

/**
 * Test that metrics calculation succeeds for a full clear pipeline path.
 */
TEST_F(GstProfilerTests, CalculateMetricsReturnsValuesForNonEncryptedPlayback)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    addRecordAndWait("Pipeline Created");
    addRecordAndWait("All Sources Attached");
    addRecordAndWait("First Segment Received", "Video");
    addRecordAndWait("First Segment Received", "Audio");
    addRecordAndWait("Source FB Exit", "Video");
    addRecordAndWait("Source FB Exit", "Audio");
    addRecordAndWait("Decoder FB Exit", "Video");
    addRecordAndWait("Decoder FB Exit", "Audio");
    addRecordAndWait("Pipeline State Changed", "PAUSED");
    addRecordAndWait("Pipeline State Changed", "PLAYING");

    const auto metrics = m_gstProfiler->calculateMetrics();
    ASSERT_TRUE(metrics.has_value());
    EXPECT_TRUE(metrics->preparation.has_value());
    EXPECT_TRUE(metrics->videoDownload.has_value());
    EXPECT_TRUE(metrics->audioDownload.has_value());
    EXPECT_TRUE(metrics->videoSource.has_value());
    EXPECT_TRUE(metrics->audioSource.has_value());
    EXPECT_TRUE(metrics->videoDecode.has_value());
    EXPECT_TRUE(metrics->audioDecode.has_value());
    EXPECT_TRUE(metrics->preRoll.has_value());
    EXPECT_TRUE(metrics->play.has_value());
    EXPECT_TRUE(metrics->total.has_value());
    EXPECT_TRUE(metrics->totalWithoutApp.has_value());
}

/**
 * Test that public API can be called multiple times.
 */
TEST_F(GstProfilerTests, MultipleCalls)
{
    setProfilerEnabledEnv(true);
    createGstProfiler();

    expectElementRecognizedAsSource(m_realElement);

    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(m_realElement, StrEq("src"))).WillOnce(Return(nullptr));

    EXPECT_NO_THROW({
        [[maybe_unused]] const auto id1 = m_gstProfiler->createRecord("PipelineCreated");
        [[maybe_unused]] const auto id2 = m_gstProfiler->createRecord("AllSourcesAttached", "audio+video");
        m_gstProfiler->logRecord(1);
        m_gstProfiler->scheduleGstElementRecord(m_realElement);
        m_gstProfiler->logPipeline();
    });
}
