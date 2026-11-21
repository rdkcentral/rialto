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

#include "VideoFrame.h"
#include "FrameImporterMock.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

TEST(VideoFrameTest, ShouldReportFrameDescription)
{
    CaptureFrameDescription description;
    description.poolId = 2;
    description.slotIndex = 1;
    description.acquisitionToken = 42;
    description.presentationTimeNs = 123456789;
    description.backingWidth = 1920;
    description.backingHeight = 1088;
    description.visibleWidth = 1280;
    description.visibleHeight = 720;

    const std::vector<std::uint32_t> kTextures{7};
    auto image = std::make_shared<NiceMock<ImportedImageMock>>();
    ON_CALL(*image, textureTarget()).WillByDefault(Return(0x8D65)); // GL_TEXTURE_EXTERNAL_OES
    ON_CALL(*image, textureIds()).WillByDefault(ReturnRef(kTextures));

    const VideoFrame kFrame{description, std::make_shared<ImportCacheEntry>(ImportCacheKey{}, image, nullptr)};
    EXPECT_EQ(kFrame.frameIndex(), 1u);
    EXPECT_EQ(kFrame.presentationTimeStampNs(), 123456789);
    EXPECT_EQ(kFrame.textureWidth(), 1920u);
    EXPECT_EQ(kFrame.textureHeight(), 1088u);
    EXPECT_EQ(kFrame.visibleWidth(), 1280u);
    EXPECT_EQ(kFrame.visibleHeight(), 720u);
    EXPECT_EQ(kFrame.layout(), GraphicsTextureLayout::RGBA);
    EXPECT_EQ(kFrame.numResources(), 1u);
    EXPECT_EQ(kFrame.textureTarget(), 0x8D65u);
    EXPECT_EQ(kFrame.textureIds(), kTextures);
    EXPECT_EQ(kFrame.description().acquisitionToken, 42u); // the lease identity
    EXPECT_EQ(kFrame.description().poolId, 2u);
}
