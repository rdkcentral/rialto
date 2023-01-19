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

#include "tasks/webAudio/SetVolume.h"
#include "WebAudioPlayerContext.h"
#include "GstWrapperMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrictMock;

class WebAudioSetVolumeTest : public testing::Test
{
public:
    firebolt::rialto::server::WebAudioPlayerContext m_context;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_pipeline{};
};

TEST_F(WebAudioSetVolumeTest, shouldSetVolume)
{
    constexpr double kVolume{0.7};
    m_context.pipeline = &m_pipeline;
    EXPECT_CALL(*m_gstWrapper, gstStreamVolumeSetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR, kVolume));
    firebolt::rialto::server::webaudio::SetVolume task{m_context, m_gstWrapper, kVolume};
    task.execute();
}
