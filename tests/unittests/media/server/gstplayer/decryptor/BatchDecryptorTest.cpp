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

#include "GlibWrapperFactoryMock.h"
#include "GlibWrapperMock.h"
#include "GstBatchDecryptorPrivate.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include <array>
#include <gst/base/gstbasetransform.h>
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAreArray;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

namespace
{
/**
 * GstRialtoBatchDecryptorPrivate with mocked decryption, so that only the batching logic is tested.
 * Decryption itself is covered by RialtoServerDecryptorPrivateDecryptMultiTest.
 */
class GstRialtoBatchDecryptorPrivateTestable : public GstRialtoBatchDecryptorPrivate
{
public:
    using GstRialtoBatchDecryptorPrivate::GstRialtoBatchDecryptorPrivate;
    MOCK_METHOD(GstFlowReturn, decryptMulti, (std::vector<GstBuffer *> bufferVec, GstCaps *caps), (override));
};

constexpr size_t kBatchSize{GstRialtoBatchDecryptorPrivate::kMaxBatchSize};
constexpr size_t kNumOfBuffers{kBatchSize + 8};
} // namespace

class RialtoServerBatchDecryptorPrivateTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock{
        std::make_shared<StrictMock<GstWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<GlibWrapperFactoryMock>> m_glibWrapperFactoryMock{
        std::make_shared<StrictMock<GlibWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock{std::make_shared<StrictMock<GlibWrapperMock>>()};

    GstElement m_decryptorElement{};
    GstObject *m_parent{reinterpret_cast<GstObject *>(&m_decryptorElement)};
    GstPad m_srcPad{};
    GstPad m_sinkPad{};
    std::array<GstBuffer, kNumOfBuffers> m_buffers{};
    int m_dummyMemory{0};
    GstBufferList *m_bufferList{reinterpret_cast<GstBufferList *>(&m_dummyMemory)};
    GstCaps m_caps{};
    GstEvent m_event{};

    std::unique_ptr<StrictMock<GstRialtoBatchDecryptorPrivateTestable>> m_sut;

    RialtoServerBatchDecryptorPrivateTest()
    {
        EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));
        EXPECT_CALL(*m_glibWrapperFactoryMock, getGlibWrapper()).WillOnce(Return(m_glibWrapperMock));

        m_sut = std::make_unique<StrictMock<GstRialtoBatchDecryptorPrivateTestable>>(&m_decryptorElement,
                                                                                     m_gstWrapperFactoryMock,
                                                                                     m_glibWrapperFactoryMock);
        m_sut->setSrcPad(&m_srcPad);
    }

    std::vector<GstBuffer *> getBuffers(size_t first, size_t count)
    {
        std::vector<GstBuffer *> buffers;
        for (size_t i = first; i < first + count; ++i)
        {
            buffers.push_back(&m_buffers[i]);
        }
        return buffers;
    }

    GstEvent *createEvent(GstEventType type)
    {
        m_event.type = type;
        return &m_event;
    }

    void expectReadBufferList(size_t count)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstBufferListLength(m_bufferList)).WillOnce(Return(count));
        for (size_t i = 0; i < count; ++i)
        {
            EXPECT_CALL(*m_gstWrapperMock, gstBufferListGet(m_bufferList, i)).WillOnce(Return(&m_buffers[i]));
            EXPECT_CALL(*m_gstWrapperMock, gstBufferRef(&m_buffers[i])).WillOnce(Return(&m_buffers[i]));
        }
        EXPECT_CALL(*m_gstWrapperMock, gstBufferListUnref(m_bufferList));
    }

    void expectDecrypt(size_t first, size_t count, GstFlowReturn result, GstCaps *caps = nullptr)
    {
        EXPECT_CALL(*m_sut, decryptMulti(ElementsAreArray(getBuffers(first, count)), caps)).WillOnce(Return(result));
    }

    void expectPush(size_t first, size_t count)
    {
        for (size_t i = first; i < first + count; ++i)
        {
            EXPECT_CALL(*m_gstWrapperMock, gstPadPush(&m_srcPad, &m_buffers[i])).WillOnce(Return(GST_FLOW_OK));
        }
    }

    void expectUnref(size_t first, size_t count)
    {
        for (size_t i = first; i < first + count; ++i)
        {
            EXPECT_CALL(*m_gstWrapperMock, gstBufferUnref(&m_buffers[i]));
        }
    }

    void expectCapsEvent()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstEventParseCaps(&m_event, _)).WillOnce(SetArgPointee<1>(&m_caps));
        EXPECT_CALL(*m_gstWrapperMock, gstCapsReplace(_, &m_caps))
            .WillOnce(Invoke(
                [](GstCaps **oldCaps, GstCaps *newCaps)
                {
                    *oldCaps = newCaps;
                    return TRUE;
                }));
        EXPECT_CALL(*m_gstWrapperMock, gstPadSetCaps(&m_srcPad, &m_caps)).WillOnce(Return(TRUE));
        EXPECT_CALL(*m_gstWrapperMock, gstEventUnref(&m_event));
    }
};

/**
 * Test that a buffer list is decrypted in one batch and pushed.
 */
TEST_F(RialtoServerBatchDecryptorPrivateTest, ShouldDecryptAndPushBufferList)
{
    expectReadBufferList(3);
    expectDecrypt(0, 3, GST_FLOW_OK);
    expectPush(0, 3);

    EXPECT_EQ(GST_FLOW_OK, m_sut->chainList(m_bufferList));
}

/**
 * Test that an empty buffer list is released without decryption.
 */
TEST_F(RialtoServerBatchDecryptorPrivateTest, ShouldHandleEmptyBufferList)
{
    expectReadBufferList(0);

    EXPECT_EQ(GST_FLOW_OK, m_sut->chainList(m_bufferList));
}

/**
 * Test that a buffer list longer than the batch size is split into batches.
 */
TEST_F(RialtoServerBatchDecryptorPrivateTest, ShouldSplitBufferListIntoBatches)
{
    expectReadBufferList(kNumOfBuffers);
    {
        InSequence s;
        expectDecrypt(0, kBatchSize, GST_FLOW_OK);
        expectDecrypt(kBatchSize, kNumOfBuffers - kBatchSize, GST_FLOW_OK);
    }
    expectPush(0, kNumOfBuffers);

    EXPECT_EQ(GST_FLOW_OK, m_sut->chainList(m_bufferList));
}

/**
 * Test that buffers dropped by decryption are released and the flow continues.
 */
TEST_F(RialtoServerBatchDecryptorPrivateTest, ShouldReleaseBuffersDroppedByDecryption)
{
    expectReadBufferList(3);
    expectDecrypt(0, 3, GST_BASE_TRANSFORM_FLOW_DROPPED);
    expectUnref(0, 3);

    EXPECT_EQ(GST_FLOW_OK, m_sut->chainList(m_bufferList));
}

/**
 * Test that buffers are released and the error is returned when decryption fails with flow error.
 */
TEST_F(RialtoServerBatchDecryptorPrivateTest, ShouldReleaseBuffersWhenDecryptionFails)
{
    expectReadBufferList(3);
    expectDecrypt(0, 3, GST_FLOW_ERROR);
    expectUnref(0, 3);

    EXPECT_EQ(GST_FLOW_ERROR, m_sut->chainList(m_bufferList));
}

/**
 * Test that buffers not pushed yet are released when push fails.
 */
TEST_F(RialtoServerBatchDecryptorPrivateTest, ShouldReleaseNotPushedBuffersWhenPushFails)
{
    expectReadBufferList(3);
    expectDecrypt(0, 3, GST_FLOW_OK);
    expectPush(0, 1);
    EXPECT_CALL(*m_gstWrapperMock, gstPadPush(&m_srcPad, &m_buffers[1])).WillOnce(Return(GST_FLOW_FLUSHING));
    expectUnref(2, 1);

    EXPECT_EQ(GST_FLOW_FLUSHING, m_sut->chainList(m_bufferList));
}

/**
 * Test that remaining buffers of the list are released when the first batch fails.
 */
TEST_F(RialtoServerBatchDecryptorPrivateTest, ShouldReleaseRemainingListBuffersWhenBatchFails)
{
    expectReadBufferList(kNumOfBuffers);
    expectDecrypt(0, kBatchSize, GST_FLOW_ERROR);
    expectUnref(0, kNumOfBuffers);

    EXPECT_EQ(GST_FLOW_ERROR, m_sut->chainList(m_bufferList));
}

/**
 * Test that caps received in caps event are used for decryption.
 */
TEST_F(RialtoServerBatchDecryptorPrivateTest, ShouldDecryptWithCapsFromCapsEvent)
{
    expectCapsEvent();
    EXPECT_TRUE(m_sut->sinkEvent(&m_sinkPad, m_parent, createEvent(GST_EVENT_CAPS)));

    expectReadBufferList(1);
    expectDecrypt(0, 1, GST_FLOW_OK, &m_caps);
    expectPush(0, 1);
    EXPECT_EQ(GST_FLOW_OK, m_sut->chainList(m_bufferList));

    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_caps));
    m_sut.reset();
}

/**
 * Test that events other than caps are forwarded.
 */
TEST_F(RialtoServerBatchDecryptorPrivateTest, ShouldForwardOtherEvents)
{
    EXPECT_CALL(*m_gstWrapperMock, gstPadEventDefault(&m_sinkPad, m_parent, &m_event)).WillOnce(Return(FALSE));
    EXPECT_FALSE(m_sut->sinkEvent(&m_sinkPad, m_parent, createEvent(GST_EVENT_SEGMENT)));
}
