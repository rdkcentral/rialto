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

#include "MediaPipelineCapabilitiesIpc.h"
#include "IpcModuleBase.h"
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::WithArgs;

class MediaPipelineCapabilitiesTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<IMediaPipelineCapabilities> m_sut;

    void createMediaPipelineCapabilitiesIpc()
    {
        expectInitIpc();

        EXPECT_NO_THROW(m_sut = std::make_unique<MediaPipelineCapabilitiesIpc>(m_ipcClientFactoryMock));
    }

    std::vector<std::string> m_mimeTypes = {"video/h264", "video/h265"};

public:
    void setGetSupportedMimeTypesResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSupportedMimeTypesResponse *getSupportedMimeTypesResponse =
            dynamic_cast<firebolt::rialto::GetSupportedMimeTypesResponse *>(response);
        for (const std::string &mimeType : m_mimeTypes)
        {
            getSupportedMimeTypesResponse->add_mime_types(mimeType);
        }
    }

    void setIsMimeTypeSupportedResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::IsMimeTypeSupportedResponse *isMimeTypeSupportedResponse =
            dynamic_cast<firebolt::rialto::IsMimeTypeSupportedResponse *>(response);
        isMimeTypeSupportedResponse->set_is_supported(true);
    }
};

TEST_F(MediaPipelineCapabilitiesTest, createMediaPipelineCapabilitiesIpc)
{
    createMediaPipelineCapabilitiesIpc();
}

TEST_F(MediaPipelineCapabilitiesTest, createMediaPipelineCapabilitiesTestFailure)
{
    expectInitIpcFailure();
    EXPECT_THROW(m_sut = std::make_unique<MediaPipelineCapabilitiesIpc>(m_ipcClientFactoryMock), std::runtime_error);
}

TEST_F(MediaPipelineCapabilitiesTest, createMediaPipelineCapabilitiesTestAttachChannelFailure)
{
    expectInitIpcButAttachChannelFailure();
    EXPECT_THROW(m_sut = std::make_unique<MediaPipelineCapabilitiesIpc>(m_ipcClientFactoryMock), std::runtime_error);
}

TEST_F(MediaPipelineCapabilitiesTest, GetSupportedMimeTypesSuccess)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedMimeTypes"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesTest::setGetSupportedMimeTypesResponse)));

    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), m_mimeTypes);
}

TEST_F(MediaPipelineCapabilitiesTest, GetSupportedMimeTypesDisconnected)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallDisconnected();

    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), std::vector<std::string>{});
}

TEST_F(MediaPipelineCapabilitiesTest, GetSupportedMimeTypesDisconnectedReconnectChannel)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedMimeTypes"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesTest::setGetSupportedMimeTypesResponse)));

    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), m_mimeTypes);
}

TEST_F(MediaPipelineCapabilitiesTest, GetSupportedMimeTypesFailure)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedMimeTypes"), _, _, _, _));

    EXPECT_EQ(m_sut->getSupportedMimeTypes(MediaSourceType::VIDEO), std::vector<std::string>{});
}

TEST_F(MediaPipelineCapabilitiesTest, IsMimeTypeSupportedSuccess)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isMimeTypeSupported"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesTest::setIsMimeTypeSupportedResponse)));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(MediaPipelineCapabilitiesTest, IsMimeTypeSupportedsDisconnected)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallDisconnected();

    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(MediaPipelineCapabilitiesTest, IsMimeTypeSupportedDisconnectedReconnectChannel)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isMimeTypeSupported"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineCapabilitiesTest::setIsMimeTypeSupportedResponse)));

    EXPECT_TRUE(m_sut->isMimeTypeSupported("video/h264"));
}

TEST_F(MediaPipelineCapabilitiesTest, IsMimeTypeSupportedFailure)
{
    createMediaPipelineCapabilitiesIpc();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isMimeTypeSupported"), _, _, _, _));

    EXPECT_FALSE(m_sut->isMimeTypeSupported("video/h264"));
}