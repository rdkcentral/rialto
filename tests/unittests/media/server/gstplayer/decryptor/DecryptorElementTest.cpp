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
#include "GstDecryptorElementFactory.h"
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include "IFactoryAccessor.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{
constexpr bool kBatchDecryption{true};
const gchar *kDecryptorName{"rialtodecryptor"};
} // namespace

class RialtoServerDecryptorElementTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock{
        std::make_shared<StrictMock<GstWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<GlibWrapperFactoryMock>> m_glibWrapperFactoryMock{
        std::make_shared<StrictMock<GlibWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock{std::make_shared<StrictMock<GlibWrapperMock>>()};
    GstDecryptorElementFactory m_sut;
    GstElement *m_element{nullptr};

    RialtoServerDecryptorElementTest()
    {
        IFactoryAccessor::instance().gstWrapperFactory() = m_gstWrapperFactoryMock;
        IFactoryAccessor::instance().glibWrapperFactory() = m_glibWrapperFactoryMock;
    }

    ~RialtoServerDecryptorElementTest() override
    {
        if (m_element)
        {
            gst_object_unref(m_element);
        }
        IFactoryAccessor::instance().gstWrapperFactory() = nullptr;
        IFactoryAccessor::instance().glibWrapperFactory() = nullptr;
    }

    void expectInitDecryptorPrivate()
    {
        EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));
        EXPECT_CALL(*m_glibWrapperFactoryMock, getGlibWrapper()).WillOnce(Return(m_glibWrapperMock));
    }

    void createElement(bool batchDecryption)
    {
        expectInitDecryptorPrivate();
        m_element = m_sut.createDecryptorElement(nullptr, nullptr, m_gstWrapperMock, batchDecryption);
        ASSERT_NE(m_element, nullptr);
        gst_object_ref_sink(m_element);
    }
};

/**
 * Test that the decryptor element is created when batch decryption is disabled.
 */
TEST_F(RialtoServerDecryptorElementTest, ShouldCreateDecryptorElement)
{
    createElement(!kBatchDecryption);
}

/**
 * Test that the batch decryptor element is created with sink and src pads when batch decryption is enabled.
 */
TEST_F(RialtoServerDecryptorElementTest, ShouldCreateBatchDecryptorElement)
{
    createElement(kBatchDecryption);

    GstPad *sinkPad = gst_element_get_static_pad(m_element, "sink");
    GstPad *srcPad = gst_element_get_static_pad(m_element, "src");
    EXPECT_NE(sinkPad, nullptr);
    EXPECT_NE(srcPad, nullptr);
    gst_object_unref(sinkPad);
    gst_object_unref(srcPad);
}

/**
 * Test that the decryptor element gets the requested name.
 */
TEST_F(RialtoServerDecryptorElementTest, ShouldCreateBatchDecryptorElementWithName)
{
    expectInitDecryptorPrivate();
    EXPECT_CALL(*m_gstWrapperMock, gstObjectSetName(_, kDecryptorName)).WillOnce(Return(TRUE));

    m_element = m_sut.createDecryptorElement(kDecryptorName, nullptr, m_gstWrapperMock, kBatchDecryption);
    ASSERT_NE(m_element, nullptr);
    gst_object_ref_sink(m_element);
}

/**
 * Test that the decryptor element is not created when setting the name fails.
 */
TEST_F(RialtoServerDecryptorElementTest, ShouldFailToCreateBatchDecryptorElementWhenSetNameFails)
{
    expectInitDecryptorPrivate();
    EXPECT_CALL(*m_gstWrapperMock, gstObjectSetName(_, kDecryptorName)).WillOnce(Return(FALSE));

    EXPECT_EQ(m_sut.createDecryptorElement(kDecryptorName, nullptr, m_gstWrapperMock, kBatchDecryption), nullptr);
}

/**
 * Test that a buffer list received on the sink pad is handled by the batch decryptor.
 */
TEST_F(RialtoServerDecryptorElementTest, ShouldHandleBufferListOnSinkPad)
{
    createElement(kBatchDecryption);
    GstPad *sinkPad = gst_element_get_static_pad(m_element, "sink");
    ASSERT_NE(sinkPad, nullptr);

    int dummyMemory{0};
    GstBufferList *bufferList{reinterpret_cast<GstBufferList *>(&dummyMemory)};
    EXPECT_CALL(*m_gstWrapperMock, gstBufferListLength(bufferList)).WillOnce(Return(0));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferListUnref(bufferList));

    EXPECT_EQ(GST_FLOW_OK, GST_PAD_CHAINLISTFUNC(sinkPad)(sinkPad, GST_OBJECT(m_element), bufferList));
    gst_object_unref(sinkPad);
}

/**
 * Test that an event received on the sink pad is handled by the batch decryptor.
 */
TEST_F(RialtoServerDecryptorElementTest, ShouldHandleEventOnSinkPad)
{
    createElement(kBatchDecryption);
    GstPad *sinkPad = gst_element_get_static_pad(m_element, "sink");
    ASSERT_NE(sinkPad, nullptr);

    GstEvent *event = gst_event_new_eos();
    EXPECT_CALL(*m_gstWrapperMock, gstPadEventDefault(sinkPad, GST_OBJECT(m_element), event)).WillOnce(Return(TRUE));

    EXPECT_TRUE(GST_PAD_EVENTFUNC(sinkPad)(sinkPad, GST_OBJECT(m_element), event));
    gst_event_unref(event);
    gst_object_unref(sinkPad);
}
