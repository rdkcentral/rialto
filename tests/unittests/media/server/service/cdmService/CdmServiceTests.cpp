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

TEST_F(CdmServiceTests, incrementSessionUsage)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    incrementSessionIdUsageCounter();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, deccrementSessionUsage)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    decrementSessionIdUsageCounter();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, incrementSessionUsageFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    incrementSessionIdUsageCounterFails();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, deccrementSessionUsageFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    decrementSessionIdUsageCounterFails();
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
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetCdmKeySessionIdWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    getCdmKeySessionIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGetCdmKeySessionIdWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    getCdmKeySessionIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGetCdmKeySessionIdWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetCdmKeySessionIdWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    getCdmKeySessionIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldDecrypt)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
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
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    mediaKeysWillDecryptWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToDecryptWhenMediaKeysIsNotFoundForSession)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    decryptShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldSelectKeyId)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    mediaKeysWillSelectKeyIdWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    selectKeyIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToSelectKeyIdWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    selectKeyIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToSelectKeyIdWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    mediaKeysWillSelectKeyIdWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    selectKeyIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToSelectKeyIdWhenMediaKeysIsNotFoundForSession)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    selectKeyIdShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldCheckIfKeyIsPresentAndReturnTrue)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCheckIfKeyIsPresent(true);
    containsKeyShouldReturn(true);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldCheckIfKeyIsPresentAndReturnFalse)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCheckIfKeyIsPresent(false);
    containsKeyShouldReturn(false);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldCheckIfKeyIsPresentAndReturnFalseWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    containsKeyShouldReturn(false);
}

TEST_F(CdmServiceTests, shouldSetDrmHeader)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillSetDrmHeaderWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    setDrmHeaderShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToSetDrmHeaderWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    setDrmHeaderShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToSetDrmHeaderWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillSetDrmHeaderWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    setDrmHeaderShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldDeleteDrmStore)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillDeleteDrmStoreWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    deleteDrmStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToDeleteDrmStoreWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    deleteDrmStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToDeleteDrmStoreWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillDeleteDrmStoreWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    deleteDrmStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldDeleteKeyStore)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillDeleteKeyStoreWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    deleteKeyStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToDeleteKeyStoreWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    deleteKeyStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToDeleteKeyStoreWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillDeleteKeyStoreWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    deleteKeyStoreShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetDrmStoreHash)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetDrmStoreHashWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    getDrmStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGetDrmStoreHashWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    getDrmStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGetDrmStoreHashWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetDrmStoreHashWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    getDrmStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetKeyStoreHash)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetKeyStoreHashWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    getKeyStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGetKeyStoreHashWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    getKeyStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGetKeyStoreHashWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetKeyStoreHashWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    getKeyStoreHashShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetLdlSessionsLimit)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetLdlSessionsLimitWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    getLdlSessionsLimitShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGetLdlSessionsLimitWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    getLdlSessionsLimitShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGetLdlSessionsLimitWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetLdlSessionsLimitWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    getLdlSessionsLimitShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetLastDrmError)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetLastDrmErrorWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    getLastDrmErrorShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGetLastDrmErrorWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    getLastDrmErrorShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGetLastDrmErrorWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetLastDrmErrorWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    getLastDrmErrorShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetDrmTimeWithStatus)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetDrmTimeWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    getDrmTimeShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGetDrmTimeWithStatusWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    getDrmTimeShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGetDrmTimeWithStatusWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetDrmTimeWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    getDrmTimeShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldReleaseKeySession)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillReleaseKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    releaseKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToReleaseKeySessionWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    releaseKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToReleaseKeySessionWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillReleaseKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    releaseKeySessionShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
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

TEST_F(CdmServiceTests, shouldGetServerCertificateNotSupportedInInactiveState)
{
    supportsServerCertificateReturnFalse();
}

TEST_F(CdmServiceTests, shouldGetServerCertificateNotSupportedIfCreationFailureInActiveState)
{
    triggerSwitchToActiveSuccess();
    mediaKeysCapabilitiesFactoryWillReturnNullptr();
    supportsServerCertificateReturnFalse();
}

TEST_F(CdmServiceTests, shouldGetServerCertificateSupportedIfSupportedInActiveState)
{
    triggerSwitchToActiveSuccess();
    mediaKeysCapabilitiesFactoryWillCreateMediaKeysCapabilities();
    supportsServerCertificateWillReturnTrue();
    supportsServerCertificateReturnTrue();
}

TEST_F(CdmServiceTests, shouldCheckThatKeySystemIsPlayready)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    mediaKeysWillCheckIfKeySystemIsPlayready(true);
    isNetflixPlayreadyKeySystemShouldReturn(true);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldReturnFalseWhenCheckingPlayreadyKeySystemWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    isNetflixPlayreadyKeySystemShouldReturn(false);
}

TEST_F(CdmServiceTests, shouldReturnFalseWhenCheckingPlayreadyKeySystemWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    mediaKeysWillCheckIfKeySystemIsPlayready(false);
    isNetflixPlayreadyKeySystemShouldReturn(false);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldReturnFalseWhenCheckingPlayreadyKeySystemWhenMediaKeysIsNotFoundForSession)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    isNetflixPlayreadyKeySystemShouldReturn(false);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldPing)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillPing();
    triggerPing();
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldGetMetricSystemData)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetMetricSystemDataWithStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    getMetricSystemDataShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::OK);
    destroyMediaKeysShouldSucceed();
}

TEST_F(CdmServiceTests, shouldFailToGetMetricSystemDataWhenNoMediaKeys)
{
    triggerSwitchToActiveSuccess();
    getMetricSystemDataShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::FAIL);
}

TEST_F(CdmServiceTests, shouldFailToGetMetricSystemDataWhenMediaKeysFails)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillGetMetricSystemDataWithStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    getMetricSystemDataShouldReturnStatus(firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE);
    destroyMediaKeysShouldSucceed();
}
