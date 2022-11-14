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

#include "CdmServiceTestsFixture.h"

TEST_F(CdmServiceTests, shouldFailToCreateMediaKeysInInactiveState)
{
    createMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldFailToCreateMediaKeysAfterSwitchToInactive)
{
    triggerSwitchToActiveSuccess();
    triggerSwitchToInactive();
    createMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldFailToCreateMediaKeysWhenFactoryReturnsNull)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillReturnNullptr();
    createMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldCreateMediaKeys)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToCreateMediaKeysWithTheSameHandleTwice)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    createMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldFailToDestroyNotExistingMediaKeys)
{
    triggerSwitchToActiveSuccess();
    destroyMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldDestroyMediaKeys)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldDestroyMediaKeysWhenSwitchedToInactive)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    triggerSwitchToInactive();
    destroyMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldCreateKeySession)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToCreateKeySessionWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    createKeySessionShouldFailWithReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToCreateKeySessionWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    createKeySessionShouldFailWithReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToCreateKeySessionWhenMediaKeysClientExists)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    mediaKeysWillRemoveKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldFailWithReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldGenerateRequest)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGenerateRequestWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    generateRequestShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGenerateRequestWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    generateRequestShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGenerateRequestWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGenerateRequestWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    generateRequestShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldLoadSession)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillLoadSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    loadSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToLoadSessionWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    loadSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToLoadSessionWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillLoadSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    loadSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldUpdateSession)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillUpdateSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    updateSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToUpdateSessionWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    updateSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToUpdateSessionWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillUpdateSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    updateSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldCloseKeySession)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCloseKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    closeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToCloseKeySessionWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    closeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToCloseKeySessionWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCloseKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    closeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldRemoveKeySession)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillRemoveKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    removeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToRemoveKeySessionWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    removeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToRemoveKeySessionWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillRemoveKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    removeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetCdmKeySessionId)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetCdmKeySessionIdWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    getCdmKeySessionIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGetCdmKeySessionIdWhenNoMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    getCdmKeySessionIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGetCdmKeySessionIdWhenMediaKeysFails)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetCdmKeySessionIdWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    getCdmKeySessionIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldDecrypt)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillDecryptWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToDecryptWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToDecryptWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillDecryptWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToDecryptWhenMediaKeysIsNotFoundForSession)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillNotFindMediaKeySession();
    decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetNoKeySystemsFromGetSupportedKeySystemsInInactiveState)
{
    getSupportedKeySystemsReturnNon();
}

TEST_F(CdmServiceTests, shouldGetNoKeySystemsFromGetSupportedKeySystemsIfCreationFailureInActiveState)
{
    triggerSwitchToActiveSuccess();
    mediaKeysCapabilitiesFactoryWillReturnNullptr();
    getSupportedKeySystemsReturnNon();
}

TEST_F(CdmServiceTests, shouldGetSupportedKeySystemsInActiveState)
{
    triggerSwitchToActiveSuccess();
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    getSupportedKeySystemsWillReturnKeySystems();
    getSupportedKeySystemsShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetKeySystemNotSupportedInInactiveState)
{
    supportsKeySystemReturnFalse();
}

TEST_F(CdmServiceTests, shouldGetKeySystemNotSupportedIfCreationFailureInActiveState)
{
    triggerSwitchToActiveSuccess();
    mediaKeysCapabilitiesFactoryWillReturnNullptr();
    supportsKeySystemReturnFalse();
}

TEST_F(CdmServiceTests, shouldGetKeySystemSupportedIfSupportedInActiveState)
{
    triggerSwitchToActiveSuccess();
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    supportsKeySystemWillReturnTrue();
    supportsKeySystemReturnTrue();
}

TEST_F(CdmServiceTests, shouldFailToGetSupportedKeySystemVersionInInactiveState)
{
    getSupportedKeySystemVersionShouldFail();
}

TEST_F(CdmServiceTests, shouldFailToGetSupportedKeySystemVersionIfCreationFailureInActiveState)
{
    triggerSwitchToActiveSuccess();
    mediaKeysCapabilitiesFactoryWillReturnNullptr();
    getSupportedKeySystemVersionShouldFail();
}

TEST_F(CdmServiceTests, shouldFailToGetSupportedKeySystemVersionIfApiFailureInActiveState)
{
    triggerSwitchToActiveSuccess();
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    getSupportedKeySystemVersionWillFail();
    getSupportedKeySystemVersionShouldFail();
}

TEST_F(CdmServiceTests, shouldGetSupportedKeySystemVersionInActiveState)
{
    triggerSwitchToActiveSuccess();
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    getSupportedKeySystemVersionWillSucceed();
    getSupportedKeySystemVersionShouldSucceed();
}
