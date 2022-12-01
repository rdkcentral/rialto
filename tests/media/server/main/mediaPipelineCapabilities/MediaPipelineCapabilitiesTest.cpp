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

#include "MediaPipelineCapabilities.h"
#include "GstCapabilitiesFactoryMock.h"
#include "GstCapabilitiesMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

class MediaPipelineCapabilitiesTest : public ::testing::Test
{
public:
    MediaPipelineCapabilitiesTest()
        : m_gstCapabilitiesFactoryMock{std::make_unique<StrictMock<GstCapabilitiesFactoryMock>>()},
          m_gstCapabilities{std::make_unique<StrictMock<GstCapabilitiesMock>>()},
          m_gstCapabilitiesMock{static_cast<StrictMock<GstCapabilitiesMock> *>(m_gstCapabilities.get())}
    {
    }
    ~MediaPipelineCapabilitiesTest() = default;

    void createMediaPipelineCapabilities()
    {
        EXPECT_CALL(*m_gstCapabilitiesFactoryMock, createGstCapabilities())
            .WillOnce(Return(ByMove(std::move(m_gstCapabilities))));
        EXPECT_NO_THROW(m_sut = std::make_unique<MediaPipelineCapabilities>(m_gstCapabilitiesFactoryMock));
    }

    void failToCreateMediaPipelineCapabilities()
    {
        EXPECT_CALL(*m_gstCapabilitiesFactoryMock, createGstCapabilities())
            .WillOnce(Return(ByMove(std::unique_ptr<firebolt::rialto::server::IGstCapabilities>())));

        EXPECT_THROW(m_sut = std::make_unique<MediaPipelineCapabilities>(m_gstCapabilitiesFactoryMock),
                     std::runtime_error);
    }

protected:
    std::shared_ptr<StrictMock<GstCapabilitiesFactoryMock>> m_gstCapabilitiesFactoryMock;
    std::unique_ptr<StrictMock<GstCapabilitiesMock>> m_gstCapabilities;
    StrictMock<GstCapabilitiesMock> *m_gstCapabilitiesMock;
    std::unique_ptr<IMediaPipelineCapabilities> m_sut;
};

TEST_F(MediaPipelineCapabilitiesTest, failToCreateMediaPipelineCapabilities)
{
    failToCreateMediaPipelineCapabilities();
}

TEST_F(MediaPipelineCapabilitiesTest, getSupportedMimeTypesIsSuccessful)
{
    MediaSourceType sourceType = MediaSourceType::VIDEO;
    std::vector<std::string> mimeTypes = {"video/h264", "video/h265"};

    EXPECT_CALL(*m_gstCapabilities, getSupportedMimeTypes(sourceType)).WillOnce(Return(mimeTypes));

    createMediaPipelineCapabilities();
    EXPECT_EQ(m_sut->getSupportedMimeTypes(sourceType), mimeTypes);
}

TEST_F(MediaPipelineCapabilitiesTest, isMimetypeSupported)
{
    std::string mimeType = "video/h264";

    EXPECT_CALL(*m_gstCapabilities, isMimeTypeSupported(mimeType)).WillOnce(Return(true));

    createMediaPipelineCapabilities();
    EXPECT_TRUE(m_sut->isMimeTypeSupported(mimeType));
}
