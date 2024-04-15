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

#include <gtest/gtest.h>

#include "MediaPipelineCapabilities.h"
#include "MediaPipelineCapabilitiesIpcFactoryMock.h"
#include "MediaPipelineCapabilitiesIpcMock.h"

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

class MediaPipelineCapabilitiesTest : public ::testing::Test
{
protected:
    std::shared_ptr<IMediaPipelineCapabilities> m_sut;
    std::unique_ptr<StrictMock<MediaPipelineCapabilitiesIpcMock>> m_mediaPipelineCapabilitiesIpc;
    StrictMock<MediaPipelineCapabilitiesIpcMock> *m_mediaPipelineCapabilitiesIpcMock;

    MediaPipelineCapabilitiesTest()
        : m_mediaPipelineCapabilitiesIpc{std::make_unique<StrictMock<MediaPipelineCapabilitiesIpcMock>>()},
          m_mediaPipelineCapabilitiesIpcMock(m_mediaPipelineCapabilitiesIpc.get())
    {
    }

    void createMediaPipelineCapabilitiesIpcSucceeds()
    {
        std::shared_ptr<StrictMock<MediaPipelineCapabilitiesIpcFactoryMock>> mediaPipelineCapabilitiesIpcFactoryMock =
            std::make_shared<StrictMock<MediaPipelineCapabilitiesIpcFactoryMock>>();

        EXPECT_CALL(*mediaPipelineCapabilitiesIpcFactoryMock, createMediaPipelineCapabilitiesIpc())
            .WillOnce(Return(ByMove(std::move(m_mediaPipelineCapabilitiesIpc))));

        EXPECT_NO_THROW(m_sut = std::make_shared<MediaPipelineCapabilities>(mediaPipelineCapabilitiesIpcFactoryMock));
    }
};

TEST_F(MediaPipelineCapabilitiesTest, createMediaPipelineCapabilitiesIpcSucceeds)
{
    createMediaPipelineCapabilitiesIpcSucceeds();
}

TEST_F(MediaPipelineCapabilitiesTest, createMediaPipelineCapabilitiesIpcFails)
{
    std::shared_ptr<StrictMock<MediaPipelineCapabilitiesIpcFactoryMock>> mediaPipelineCapabilitiesIpcFactoryMock =
        std::make_shared<StrictMock<MediaPipelineCapabilitiesIpcFactoryMock>>();

    EXPECT_CALL(*mediaPipelineCapabilitiesIpcFactoryMock, createMediaPipelineCapabilitiesIpc())
        .WillOnce(Return(ByMove(std::move(std::unique_ptr<StrictMock<MediaPipelineCapabilitiesIpcMock>>()))));

    EXPECT_THROW(m_sut = std::make_shared<MediaPipelineCapabilities>(mediaPipelineCapabilitiesIpcFactoryMock),
                 std::runtime_error);
}

TEST_F(MediaPipelineCapabilitiesTest, getSupportedMimeTypes)
{
    std::vector<std::string> mimeTypes = {"video/h264", "video/h265"};
    createMediaPipelineCapabilitiesIpcSucceeds();

    EXPECT_CALL(*m_mediaPipelineCapabilitiesIpcMock, getSupportedMimeTypes(MediaSourceType::VIDEO))
        .WillOnce(Return(mimeTypes));
    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), mimeTypes);
}

TEST_F(MediaPipelineCapabilitiesTest, isMimeTypeSupported)
{
    createMediaPipelineCapabilitiesIpcSucceeds();

    EXPECT_CALL(*m_mediaPipelineCapabilitiesIpcMock, isMimeTypeSupported("video/h264")).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
}

/**
 * Test the factory failure
 */
TEST_F(MediaPipelineCapabilitiesTest, CapabilitiesFactoryFails)
{
    std::shared_ptr<firebolt::rialto::IMediaPipelineCapabilitiesFactory> factory =
        firebolt::rialto::IMediaPipelineCapabilitiesFactory::createFactory();
    EXPECT_NE(factory, nullptr);

    // The following call is expected to fail because it's difficult to inject a mock
    // version of IMediaPipelineCapabilitiesIpcFactory without affecting a lot of code on both
    // the client and server
    EXPECT_EQ(factory->createMediaPipelineCapabilities(), nullptr);
}
