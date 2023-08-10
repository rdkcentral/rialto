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

#include "tasks/webAudio/Eos.h"
#include "GstWebAudioPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::Return;
using testing::StrictMock;

class WebAudioEosTest : public testing::Test
{
protected:
    firebolt::rialto::server::WebAudioPlayerContext m_context{};
    GstElement m_src{};
};

TEST_F(WebAudioEosTest, shouldSetEos)
{
    shouldEndOfStreamSuccess();
    triggerEos();
    firebolt::rialto::server::tasks::webaudio::Eos task{m_context, m_gstWrapper};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcEndOfStream(GST_APP_SRC(m_context.source))).WillOnce(Return(GST_FLOW_OK));
    task.execute();
}

TEST_F(WebAudioEosTest, shouldFailToSetEos)
{
    shouldEndOfStreamFailure();
    triggerEos();
    firebolt::rialto::server::tasks::webaudio::Eos task{m_context, m_gstWrapper};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcEndOfStream(GST_APP_SRC(m_context.source))).WillOnce(Return(GST_FLOW_ERROR));
    task.execute();
}
