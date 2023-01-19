/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "tasks/webAudio/WriteBuffer.h"
#include "WebAudioPlayerContext.h"
#include "GstWebAudioPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "GlibWrapperMock.h"
#include "Matchers.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::StrictMock;
using testing::Return;
using testing::_;

class WebAudioWriteBufferTest : public testing::Test
{
protected:
    firebolt::rialto::server::WebAudioPlayerContext m_context;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};

    uint8_t m_mainPtr{};
    uint32_t m_mainLength = firebolt::rialto::server::kMaxWebAudioBytes - 20;
    uint8_t m_wrapPtr{};
    uint32_t m_wrapLength = 20;
    GstElement m_appSrc{};

    WebAudioWriteBufferTest()
    {
        m_context.source = &m_appSrc;
    }
};

TEST_F(WebAudioWriteBufferTest, shouldWriteBuffer)
{
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&m_appSrc))).WillOnce(Return(0));
}

TEST_F(WebAudioWriteBufferTest, shouldNotWriteBufferIfNoSpace)
{
}

TEST_F(WebAudioWriteBufferTest, shouldNotWriteBufferIfBytesWrittenInvalid)
{
}

TEST_F(WebAudioWriteBufferTest, shouldNotWriteBufferIfPushBufferFails)
{
}
