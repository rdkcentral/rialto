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

#ifndef CDM_SERVICE_TESTS_FIXTURE_H_
#define CDM_SERVICE_TESTS_FIXTURE_H_

#include "CdmService.h"
#include "MediaKeysCapabilitiesFactoryMock.h"
#include "MediaKeysCapabilitiesMock.h"
#include "MediaKeysClientMock.h"
#include "MediaKeysServerInternalFactoryMock.h"
#include "MediaKeysServerInternalMock.h"
#include "SharedMemoryBufferFactoryMock.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using testing::StrictMock;

class CdmServiceTests : public testing::Test
{
public:
    CdmServiceTests();
    ~CdmServiceTests() = default;

    void mediaKeysFactoryWillCreateMediaKeys();
    void mediaKeysFactoryWillReturnNullptr();
    void mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void mediaKeysWillGenerateRequestWithStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void mediaKeysWillLoadSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void mediaKeysWillUpdateSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void mediaKeysWillCloseKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void mediaKeysWillRemoveKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void mediaKeysWillGetCdmKeySessionIdWithStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void mediaKeysWillDecryptWithStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void mediaKeysWillNotFindMediaKeySession();

    void mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    void mediaKeysCapabilitiesFactoryWillReturnNullptr();
    void getSupportedKeySystemsWillReturnKeySystems();
    void supportsKeySystemWillReturnTrue();
    void getSupportedKeySystemVersionWillSucceed();
    void getSupportedKeySystemVersionWillFail();

    void triggerSwitchToActiveSuccess();
    void triggerSwitchToInactive();

    void createMediaKeysShouldSucceed();
    void createMediaKeysShouldFail();
    void destroyMediaKeysShouldSucceed();
    void destroyMediaKeysShouldFail();
    void createKeySessionShouldSucceed();
    void createKeySessionShouldFailWithReturnStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void generateRequestShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void loadSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void updateSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void closeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void removeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void getCdmKeySessionIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status);
    void decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus status);

    void getSupportedKeySystemsShouldSucceed();
    void getSupportedKeySystemsReturnNon();
    void supportsKeySystemReturnTrue();
    void supportsKeySystemReturnFalse();
    void getSupportedKeySystemVersionShouldSucceed();
    void getSupportedKeySystemVersionShouldFail();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::server::MediaKeysServerInternalFactoryMock>> m_mediaKeysFactoryMock;
    std::unique_ptr<firebolt::rialto::server::IMediaKeysServerInternal> m_mediaKeys;
    StrictMock<firebolt::rialto::server::MediaKeysServerInternalMock> &m_mediaKeysMock;
    std::shared_ptr<StrictMock<firebolt::rialto::MediaKeysCapabilitiesFactoryMock>> m_mediaKeysCapabilitiesFactoryMock;
    std::shared_ptr<firebolt::rialto::IMediaKeysCapabilities> m_mediaKeysCapabilities;
    StrictMock<firebolt::rialto::MediaKeysCapabilitiesMock> &m_mediaKeysCapabilitiesMock;
    std::shared_ptr<StrictMock<firebolt::rialto::MediaKeysClientMock>> m_mediaKeysClientMock;
    firebolt::rialto::server::service::CdmService m_sut;
};

#endif // CDM_SERVICE_TESTS_FIXTURE_H_
