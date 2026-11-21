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

#include "IVideoFrameCapture.h"
#include "MediaPipelineTestBase.h"

using firebolt::rialto::IVideoFrameCaptureFactory;

class RialtoClientVideoFrameCaptureTest : public MediaPipelineTestBase
{
protected:
    virtual void SetUp()
    {
        MediaPipelineTestBase::SetUp();

        createMediaPipeline();
    }

    virtual void TearDown()
    {
        destroyMediaPipeline();

        MediaPipelineTestBase::TearDown();
    }
};

/**
 * Test that the factory exists and returns no GL capture until captures can be registered with the server.
 */
TEST_F(RialtoClientVideoFrameCaptureTest, FactoryReturnsNoCapture)
{
    std::shared_ptr<IVideoFrameCaptureFactory> factory = IVideoFrameCaptureFactory::createFactory();
    ASSERT_NE(factory, nullptr);
    EXPECT_EQ(factory->createGLVideoFrameCapture(nullptr, nullptr), nullptr);
}

/**
 * Test that attaching and detaching a capture fail on a pipeline, without any IPC call.
 */
TEST_F(RialtoClientVideoFrameCaptureTest, AttachAndDetachFail)
{
    EXPECT_FALSE(m_mediaPipeline->attachVideoFrameCapture(nullptr));
    EXPECT_FALSE(m_mediaPipeline->detachVideoFrameCapture());
}
