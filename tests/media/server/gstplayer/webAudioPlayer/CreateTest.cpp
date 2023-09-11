/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "GstWebAudioPlayerTestCommon.h"
#include "Matchers.h"
#include <gtest/gtest.h>
#include <memory>

class RialtoServerCreateGstWebAudioPlayerTest : public GstWebAudioPlayerTestCommon
{
protected:
    std::unique_ptr<IGstWebAudioPlayer> m_gstPlayer;
};

/**
 * Test that a GstWebAudioPlayer object can be created successfully for the llama platform.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, CreateDestroyLlamaSuccess)
{
    gstPlayerWillBeCreatedForLlama();

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                      m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                      std::move(m_taskFactory),
                                                                      std::move(workerThreadFactory),
                                                                      std::move(gstDispatcherThreadFactory)));
    EXPECT_NE(m_gstPlayer, nullptr);

    gstPlayerWillBeDestroyed();
    m_gstPlayer.reset();
}

/**
 * Test the factory
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, FactoryCreatesObject)
{
    std::shared_ptr<firebolt::rialto::server::IGstWebAudioPlayerFactory> factory =
      firebolt::rialto::server::IGstWebAudioPlayerFactory::getFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_NE(factory->createGstWebAudioPlayer(&m_gstPlayerClient, m_priority), nullptr);
}


/**
 * Test that a GstWebAudioPlayer object can be created successfully for the xione platform.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, CreateDestroyXiOneSuccess)
{
    gstPlayerWillBeCreatedForXiOne();

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                      m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                      std::move(m_taskFactory),
                                                                      std::move(workerThreadFactory),
                                                                      std::move(gstDispatcherThreadFactory)));
    EXPECT_NE(m_gstPlayer, nullptr);

    gstPlayerWillBeDestroyed();
    m_gstPlayer.reset();
}

/**
 * Test that a GstWebAudioPlayer object can be created successfully for the other platforms.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, CreateDestroyGenericPlatformsSuccess)
{
    gstPlayerWillBeCreatedForGenericPlatform();

    EXPECT_NO_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                      m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                      std::move(m_taskFactory),
                                                                      std::move(workerThreadFactory),
                                                                      std::move(gstDispatcherThreadFactory)));
    EXPECT_NE(m_gstPlayer, nullptr);

    gstPlayerWillBeDestroyed();
    m_gstPlayer.reset();
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to get the gstreamer source.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, getGstSrcFailure)
{
    EXPECT_CALL(*m_gstSrcFactoryMock, getGstSrc()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to create the worker thread.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, createWorkerThreadFailure)
{
    expectInitRialtoSrc();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to create the pipeline.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, createPipelineFailure)
{
    expectInitRialtoSrc();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineNew(_)).WillOnce(Return(nullptr));

    // Reset worker thread on failure
    expectResetWorkerThread();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to create the app src.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, createAppSrcFailure)
{
    expectInitRialtoSrc();
    expectCreatePipeline();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(_, _)).WillOnce(Return(nullptr));

    // Reset worker thread and pipeline on failure
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pipeline));
    expectTaskStop();
    expectResetWorkerThread();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to get the registry.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, getRegistryFailure)
{
    expectInitRialtoSrc();
    expectCreatePipeline();
    expectInitAppSrc();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet()).WillOnce(Return(nullptr));

    // Reset worker thread and pipeline on failure
    gstPlayerWillBeDestroyed();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to create a amlhalasink.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, createAmlhalaSinkFailure)
{
    expectInitRialtoSrc();
    expectCreatePipeline();
    expectInitAppSrc();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet()).WillOnce(Return(&m_reg));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("amlhalasink")))
        .WillOnce(Return(GST_PLUGIN_FEATURE(&m_feature)));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_PLUGIN_FEATURE(&m_feature)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("amlhalasink"), CharStrMatcher("webaudiosink")))
        .WillOnce(Return(nullptr));

    // Reset worker thread and pipeline on failure
    gstPlayerWillBeDestroyed();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to create a rtkaudiosink.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, createRtkAudioSinkFailure)
{
    expectInitRialtoSrc();
    expectCreatePipeline();
    expectInitAppSrc();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet()).WillOnce(Return(&m_reg));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("amlhalasink"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("rtkaudiosink")))
        .WillOnce(Return(GST_PLUGIN_FEATURE(&m_feature)));
    EXPECT_CALL(*m_gstWrapperMock,
                gstElementFactoryMake(CharStrMatcher("rtkaudiosink"), CharStrMatcher("webaudiosink")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_PLUGIN_FEATURE(&m_feature)));

    // Reset worker thread and pipeline on failure
    gstPlayerWillBeDestroyed();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to create audio convert.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, createAudioConvertFailure)
{
    expectInitRialtoSrc();
    expectCreatePipeline();
    expectInitAppSrc();
    expectMakeRtkAudioSink();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_sink), CharStrMatcher("media-tunnel")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_sink), CharStrMatcher("audio-service")));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("audioconvert"), _)).WillOnce(Return(nullptr));

    // Reset worker thread and pipeline on failure
    gstPlayerWillBeDestroyed();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to create audio resample.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, createAudioResampleFailure)
{
    expectInitRialtoSrc();
    expectMakeRtkAudioSink();
    expectCreatePipeline();
    expectInitAppSrc();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_sink), CharStrMatcher("media-tunnel")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_sink), CharStrMatcher("audio-service")));

    GstElement convert{};
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("audioconvert"), _)).WillOnce(Return(&convert));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("audioresample"), _)).WillOnce(Return(nullptr));

    // Reset worker thread and pipeline on failure
    gstPlayerWillBeDestroyed();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to add the audio sink elements to the bin.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, linkAudioSinkBinFailure)
{
    expectInitRialtoSrc();
    expectCreatePipeline();
    expectInitAppSrc();
    expectMakeAutoAudioSink();
    expectAddBinFailure();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));

    // Reset worker thread and pipeline on failure
    gstPlayerWillBeDestroyed();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to link audio sink elements.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, linkElementFailure)
{
    expectInitRialtoSrc();
    expectCreatePipeline();
    expectInitAppSrc();
    expectMakeAutoAudioSink();
    expectLinkElementFailure();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));

    // Reset worker thread and pipeline on failure
    gstPlayerWillBeDestroyed();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to create a autoaudiosink.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, createAutoAudioSinkFailure)
{
    expectInitRialtoSrc();
    expectCreatePipeline();
    expectInitAppSrc();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet()).WillOnce(Return(&m_reg));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("amlhalasink"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("rtkaudiosink")))
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*m_gstWrapperMock,
                gstElementFactoryMake(CharStrMatcher("autoaudiosink"), CharStrMatcher("webaudiosink")))
        .WillOnce(Return(nullptr));

    // Reset worker thread and pipeline on failure
    gstPlayerWillBeDestroyed();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}

/**
 * Test that a GstWebAudioPlayer throws an exception if there is a failure to create the dispatcher thread.
 */
TEST_F(RialtoServerCreateGstWebAudioPlayerTest, createGstDispatcherThreadFailure)
{
    expectInitRialtoSrc();
    expectCreatePipeline();
    expectInitAppSrc();
    expectAddElementsAutoAudioSink();
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(m_gstDispatcherThreadFactoryMock, createGstDispatcherThread(_, _, _)).WillOnce(Return(nullptr));

    // Reset worker thread and pipeline on failure
    gstPlayerWillBeDestroyed();

    EXPECT_THROW(m_gstPlayer = std::make_unique<GstWebAudioPlayer>(&m_gstPlayerClient, m_priority, m_gstWrapperMock,
                                                                   m_glibWrapperMock, m_gstSrcFactoryMock,
                                                                   std::move(m_taskFactory),
                                                                   std::move(workerThreadFactory),
                                                                   std::move(gstDispatcherThreadFactory)),
                 std::runtime_error);
    EXPECT_EQ(m_gstPlayer, nullptr);
}
