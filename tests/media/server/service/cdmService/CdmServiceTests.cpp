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
    mainThreadWillEnqueueTask();
    createMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldFailSwitchToActiveIfCapabilitiesAreNull)
{
    mediaKeysCapabilitiesFactoryWillReturnNullptr();
    triggerSwitchToActiveFail();
}

TEST_F(CdmServiceTests, shouldFailToCreateMediaKeysAfterSwitchToInactive)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    triggerSwitchToInactive();
    mainThreadWillEnqueueTask();
    createMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldFailToCreateMediaKeysWhenFactoryReturnsNull)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillReturnNullptr();
    createMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldCreateMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToCreateMediaKeysWithTheSameHandleTwice)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mainThreadWillEnqueueTask();
    createMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldFailToDestroyNotExistingMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldDestroyMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldDestroyMediaKeysWhenSwitchedToInactive)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mainThreadWillEnqueueTask();
    triggerSwitchToInactive();
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldFail();
}

TEST_F(CdmServiceTests, shouldCreateKeySession)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToCreateKeySessionWhenNoMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    createKeySessionShouldFailWithReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToCreateKeySessionWhenMediaKeysFails)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    createKeySessionShouldFailWithReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToCreateKeySessionWhenMediaKeysClientExists)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
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
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGenerateRequestWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    generateRequestShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGenerateRequestWhenNoMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    generateRequestShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGenerateRequestWhenMediaKeysFails)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGenerateRequestWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    generateRequestShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldLoadSession)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillLoadSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    loadSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToLoadSessionWhenNoMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    loadSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToLoadSessionWhenMediaKeysFails)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillLoadSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    loadSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldUpdateSession)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillUpdateSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    updateSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToUpdateSessionWhenNoMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    updateSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToUpdateSessionWhenMediaKeysFails)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillUpdateSessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    updateSessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldCloseKeySession)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCloseKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    closeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToCloseKeySessionWhenNoMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    closeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToCloseKeySessionWhenMediaKeysFails)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCloseKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    closeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldRemoveKeySession)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillRemoveKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    removeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToRemoveKeySessionWhenNoMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    removeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToRemoveKeySessionWhenMediaKeysFails)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillRemoveKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    removeKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    mainThreadWillEnqueueTask();
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
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillDecryptWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    mainThreadWillEnqueueTask();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToDecryptWhenNoMediaKeys)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mainThreadWillEnqueueTask();
    decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToDecryptWhenMediaKeysFails)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillDecryptWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    mainThreadWillEnqueueTask();
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
    mainThreadWillEnqueueTask();
    getSupportedKeySystemsReturnNon();
}

TEST_F(CdmServiceTests, shouldGetSupportedKeySystemsInActiveState)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    getSupportedKeySystemsWillReturnKeySystems();
    getSupportedKeySystemsShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetKeySystemNotSupportedInInactiveState)
{
    mainThreadWillEnqueueTask();
    supportsKeySystemReturnFalse();
}

TEST_F(CdmServiceTests, shouldGetKeySystemSupportedIfSupportedInActiveState)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    supportsKeySystemWillReturnTrue();
    supportsKeySystemReturnTrue();
}

TEST_F(CdmServiceTests, shouldFailToGetSupportedKeySystemVersionInInactiveState)
{
    mainThreadWillEnqueueTask();
    getSupportedKeySystemVersionShouldFail();
}

TEST_F(CdmServiceTests, shouldFailToGetSupportedKeySystemVersionIfFailureInActiveState)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    getSupportedKeySystemVersionWillFail();
    getSupportedKeySystemVersionShouldFail();
}

TEST_F(CdmServiceTests, shouldGetSupportedKeySystemVersionInActiveState)
{
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    triggerSwitchToActiveSuccess();
    getSupportedKeySystemVersionWillSucceed();
    getSupportedKeySystemVersionShouldSucceed();
}
