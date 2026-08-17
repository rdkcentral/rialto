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

#include "MediaCapabilitiesIpc.h"
#include "IpcModuleBase.h"
#include "MediaCapabilitiesIpcMock.h"

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

class MediaCapabilitiesIpcTest : public IpcModuleBase: public ::testing::Test
{
protected:
    std::unique_ptr<firebolt::rialto::IMediaCapabilities> m_sut;

    void createMediaCapabilitiesIpc()
    {
        expectInitIpc();
        EXPECT_NO_THROW(m_sut = std::make_unique<firebolt::rialto::client::MediaCapabilitiesIpc>(*m_ipcClientMock));
    }
};

TEST_F(MediaCapabilitiesIpcTest, createMediaCapabilitiesIpcSucceeds)
{
    createMediaCapabilitiesIpc();
}

TEST_F(MediaCapabilitiesIpcTest, createMediaCapabilitiesIpcAttachChannelFailure)
{
    expectInitIpcButAttachChannelFailure();
    EXPECT_THROW(m_sut = std::make_unique<firebolt::rialto::client::MediaCapabilitiesIpc>(*m_ipcClientMock), 
                 std::runtime_error);
}

TEST_F(MediaCapabilitiesIpcTest, AudioDecoderCapabilities)
{
    const firebolt::rialto::common::AudioDecoderCapabilities kExpectedCapabilities{"1.0", "2.0", {}};
    createMediaPipelineCapabilitiesIpcSucceeds();

    EXPECT_CALL(*m_mediaPipelineCapabilitiesIpcMock, getSupportedAudioCapabilities()).WillOnce(Return(kExpectedCapabilities));
    EXPECT_THAT(m_sut->getSupportedAudioCapabilities(), decoderCapabilitiesMatcher(kExpectedCapabilities));
}

TEST_F(MediaCapabilitiesIpcTest, VideoDecoderCapabilities)
{
    const firebolt::rialto::common::VideoDecoderCapabilities kExpectedCapabilities{"1.0", "2.0", {}};
    createMediaPipelineCapabilitiesIpcSucceeds();

    EXPECT_CALL(*m_mediaPipelineCapabilitiesIpcMock, getSupportedVideoCapabilities()).WillOnce(Return(kExpectedCapabilities));
    EXPECT_THAT(m_sut->getSupportedVideoCapabilities(), decoderCapabilitiesMatcher(kExpectedCapabilities));
}