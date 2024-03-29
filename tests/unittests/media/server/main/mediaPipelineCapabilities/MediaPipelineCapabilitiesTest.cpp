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
#include "GstWrapperFactoryMock.h"
#include "GstWrapperMock.h"
#include "IFactoryAccessor.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

class MediaPipelineCapabilitiesTest : public ::testing::Test
{
public:
    MediaPipelineCapabilitiesTest()
        : m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()},
          m_gstWrapperFactoryMock{std::make_shared<StrictMock<GstWrapperFactoryMock>>()},
          m_gstCapabilitiesFactoryMock{std::make_unique<StrictMock<GstCapabilitiesFactoryMock>>()},
          m_gstCapabilities{std::make_unique<StrictMock<GstCapabilitiesMock>>()},
          m_gstCapabilitiesMock{static_cast<StrictMock<GstCapabilitiesMock> *>(m_gstCapabilities.get())}
    {
        IFactoryAccessor::instance().gstWrapperFactory() = m_gstWrapperFactoryMock;
    }
    ~MediaPipelineCapabilitiesTest() override { IFactoryAccessor::instance().gstWrapperFactory() = nullptr; }

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
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock;
    std::shared_ptr<StrictMock<GstWrapperFactoryMock>> m_gstWrapperFactoryMock;
    std::shared_ptr<StrictMock<GstCapabilitiesFactoryMock>> m_gstCapabilitiesFactoryMock;
    std::unique_ptr<StrictMock<GstCapabilitiesMock>> m_gstCapabilities;
    StrictMock<GstCapabilitiesMock> *m_gstCapabilitiesMock;
    std::unique_ptr<IMediaPipelineCapabilities> m_sut;
};

TEST_F(MediaPipelineCapabilitiesTest, failToCreateMediaPipelineCapabilities)
{
    failToCreateMediaPipelineCapabilities();
}

/**
 * Test the factory
 */
TEST_F(MediaPipelineCapabilitiesTest, FactoryCreatesObject)
{
    EXPECT_CALL(*m_gstWrapperFactoryMock, getGstWrapper()).WillOnce(Return(m_gstWrapperMock));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListGetElements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL))
        .WillOnce(Return(nullptr));
    std::shared_ptr<firebolt::rialto::IMediaPipelineCapabilitiesFactory> factory =
        firebolt::rialto::IMediaPipelineCapabilitiesFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_NE(factory->createMediaPipelineCapabilities(), nullptr);
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
