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

// NOTE: IMediaCapabilities.h / IMediaCapabilitiesIpcFactory.h were not directly available, so
// mock class names/paths below (IMediaCapabilitiesMock, IMediaCapabilitiesIpcFactoryMock) follow
// this codebase's <Interface>Mock convention. Adjust include paths/names if they differ.
#include "IMediaCapabilitiesIpcFactoryMock.h"
#include "IMediaCapabilitiesMock.h"
#include "MediaCapabilities.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

// ---------------------------------------------------------------------------
// client::MediaCapabilities - fully unit-testable: the IPC factory is
// constructor-injected, not obtained via a singleton, so no real IPC
// connection is ever attempted here.
// ---------------------------------------------------------------------------
class ClientMediaCapabilitiesTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<IMediaCapabilitiesIpcFactoryMock>> m_ipcFactoryMock{
        std::make_shared<StrictMock<IMediaCapabilitiesIpcFactoryMock>>()};

    // Raw pointer kept for setting expectations before ownership transfers via unique_ptr move.
    StrictMock<IMediaCapabilitiesMock> *m_ipcMock{nullptr};
    std::unique_ptr<MediaCapabilities> m_sut;

    // Constructs m_sut with a working IPC mock. Returns the raw pointer so callers can
    // set further EXPECT_CALLs against it before construction completes.
    StrictMock<IMediaCapabilitiesMock> &createSutWithWorkingIpc()
    {
        auto ipcMock = std::make_unique<StrictMock<IMediaCapabilitiesMock>>();
        m_ipcMock = ipcMock.get();
        EXPECT_CALL(*m_ipcFactoryMock, createMediaCapabilitiesIpc()).WillOnce(Return(ByMove(std::move(ipcMock))));

        m_sut = std::make_unique<MediaCapabilities>(m_ipcFactoryMock);
        return *m_ipcMock;
    }
};

TEST_F(ClientMediaCapabilitiesTest, ConstructsSuccessfullyWhenIpcCreationSucceeds)
{
    EXPECT_NO_THROW(createSutWithWorkingIpc());
}

TEST_F(ClientMediaCapabilitiesTest, ThrowsWhenIpcCreationFails)
{
    EXPECT_CALL(*m_ipcFactoryMock, createMediaCapabilitiesIpc()).WillOnce(Return(ByMove(nullptr)));

    EXPECT_THROW(std::make_unique<MediaCapabilities>(m_ipcFactoryMock), std::runtime_error);
}

TEST_F(ClientMediaCapabilitiesTest, GetSupportedAudioCapabilitiesDelegatesToIpc)
{
    auto &ipcMock = createSutWithWorkingIpc();

    common::AudioDecoderCapabilities expected;
    expected.interfaceVersion = "1.0";
    expected.schemaVersion = "0.1.0";
    EXPECT_CALL(ipcMock, getSupportedAudioCapabilities()).WillOnce(Return(expected));

    auto result = m_sut->getSupportedAudioCapabilities();

    EXPECT_EQ(result.interfaceVersion, "1.0");
    EXPECT_EQ(result.schemaVersion, "0.1.0");
}

TEST_F(ClientMediaCapabilitiesTest, GetSupportedVideoCapabilitiesDelegatesToIpc)
{
    auto &ipcMock = createSutWithWorkingIpc();

    common::VideoDecoderCapabilities expected;
    expected.interfaceVersion = "1.0";
    expected.schemaVersion = "0.1.0";
    EXPECT_CALL(ipcMock, getSupportedVideoCapabilities()).WillOnce(Return(expected));

    auto result = m_sut->getSupportedVideoCapabilities();

    EXPECT_EQ(result.interfaceVersion, "1.0");
    EXPECT_EQ(result.schemaVersion, "0.1.0");
}

// ---------------------------------------------------------------------------
// firebolt::rialto::MediaCapabilitiesFactory - LIMITED unit-testability.
//
// createMediaCapabilities() always calls the real, static
// client::IMediaCapabilitiesIpcFactory::createFactory() (not an injectable
// parameter), which - same as MediaCapabilitiesIpcFactory in the earlier
// investigation - ultimately reaches IIpcClientAccessor::instance(), a
// hard-coded Meyer's singleton with no test seam that attempts a REAL IPC
// connection. In a unit-test environment (no live Rialto server), that
// connection attempt fails, so only the FAILURE path is realistically
// testable here without a source change adding a test seam.
// ---------------------------------------------------------------------------
class ClientMediaCapabilitiesFactoryTest : public ::testing::Test
{
};

TEST_F(ClientMediaCapabilitiesFactoryTest, StaticCreateFactoryReturnsValidFactory)
{
    // createFactory() only default-constructs MediaCapabilitiesFactory - no IPC touched yet,
    // so this part IS safely and meaningfully unit-testable.
    auto factory = IMediaCapabilitiesFactory::createFactory();

    ASSERT_NE(factory, nullptr);
}

TEST_F(ClientMediaCapabilitiesFactoryTest, CreateMediaCapabilitiesReturnsNullptrWhenNoRealIpcServerAvailable)
{
    // See class-level comment: this exercises the catch-block/failure path only.
    // The success path is not unit-testable without a test seam for IIpcClientAccessor.
    MediaCapabilitiesFactory factory;

    auto result = factory.createMediaCapabilities();

    EXPECT_EQ(result, nullptr);
}
