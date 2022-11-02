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

#include "MediaKeysCapabilities.h"
#include "MediaKeysCapabilitiesIpcFactoryMock.h"
#include "MediaKeysCapabilitiesIpcMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::client::mock;

using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

class RialtoClientCreateMediaKeysCapabilitiesTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<MediaKeysCapabilitiesIpcFactoryMock>> m_mediaKeysCapabilitiesIpcFactoryMock;

    RialtoClientCreateMediaKeysCapabilitiesTest()
        : m_mediaKeysCapabilitiesIpcFactoryMock{std::make_shared<StrictMock<MediaKeysCapabilitiesIpcFactoryMock>>()}
    {
    }
};

/**
 * Test that a MediaKeysCapabilities object can be created successfully.
 */
TEST_F(RialtoClientCreateMediaKeysCapabilitiesTest, Create)
{
    std::shared_ptr<IMediaKeysCapabilities> mediaKeysCapabilities;
    std::shared_ptr<StrictMock<MediaKeysCapabilitiesIpcMock>> mediaKeysCapabilitiesIpcMock =
        std::make_shared<StrictMock<MediaKeysCapabilitiesIpcMock>>();

    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcFactoryMock, getMediaKeysCapabilitiesIpc())
        .WillOnce(Return(ByMove(std::move(mediaKeysCapabilitiesIpcMock))));

    EXPECT_NO_THROW(
        mediaKeysCapabilities = std::make_shared<MediaKeysCapabilities>(m_mediaKeysCapabilitiesIpcFactoryMock));
    EXPECT_NE(mediaKeysCapabilities, nullptr);
}

/**
 * Test that a MediaKeysCapabilities object throws an exeption if failure occurs during construction.
 * In this case, getMediaKeysCapabilitiesIpc fails, returning a nullptr.
 */
TEST_F(RialtoClientCreateMediaKeysCapabilitiesTest, getMediaKeysCapabilitiesIpcFailure)
{
    std::shared_ptr<IMediaKeysCapabilities> mediaKeysCapabilities;

    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcFactoryMock, getMediaKeysCapabilitiesIpc()).WillOnce(Return(ByMove(nullptr)));

    EXPECT_THROW(mediaKeysCapabilities = std::make_shared<MediaKeysCapabilities>(m_mediaKeysCapabilitiesIpcFactoryMock),
                 std::runtime_error);
}
