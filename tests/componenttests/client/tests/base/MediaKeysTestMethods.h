/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef FIREBOLT_RIALTO_CLIENT_CT_MEDIA_KEYS_TEST_METHODS_H_
#define FIREBOLT_RIALTO_CLIENT_CT_MEDIA_KEYS_TEST_METHODS_H_

#include "IMediaKeys.h"
#include "MediaKeysClientMock.h"
#include "MediaKeysModuleMock.h"
#include "ServerStub.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

using namespace firebolt::rialto;

namespace firebolt::rialto::client::ct
{
class MediaKeysTestMethods
{
public:
    MediaKeysTestMethods();
    virtual ~MediaKeysTestMethods();

protected:
    // Strict Mocks
    std::shared_ptr<StrictMock<MediaKeysClientMock>> m_mediaKeysClientMock;
    std::shared_ptr<StrictMock<MediaKeysModuleMock>> m_mediaKeysModuleMock;

    // Objects
    std::shared_ptr<IMediaKeysFactory> m_mediaKeysFactory;
    std::shared_ptr<IMediaKeys> m_mediaKeys;

    // Test methods
    void shouldCreateMediaKeysWidevine();
    void createMediaKeysWidevine();
    void shouldCreateMediaKeysPlayready();
    void createMediaKeysPlayready();
    void shouldCreateKeySession();
    void shouldCreateKeySessionFailure();
    void createKeySession();
    void createKeySessionFailure();
    void shouldGenerateRequest();
    void shouldGenerateRequestFailure();
    void shouldGenerateRequestAndSendNotifyLicenseRequest();
    void generateRequest();
    void generateRequestFailure();
    void shouldNotifyLicenseRequest();
    void sendNotifyLicenseRequest();
    void shouldNotifyKeyStatusesChanged();
    void sendNotifyKeyStatusesChanged();
    void shouldUpdateSession();
    void shouldUpdateSessionFailure();
    void updateSession();
    void updateSessionFailure();
    void shouldCloseKeySession();
    void shouldCloseKeySessionFailure();
    void closeKeySession();
    void closeKeySessionFailure();
    void shouldDestroyMediaKeys();
    void destroyMediaKeys();
    void initaliseMediaKeySession();
    void terminateMediaKeySession();

    // Component test helpers
    virtual void notifyEvent() = 0;
    virtual void waitEvent() = 0;
    virtual std::shared_ptr<ServerStub> &getServerStub() = 0;
    virtual int32_t getShmFd() = 0;
    virtual void *getShmAddress() = 0;
    virtual uint32_t getShmSize() = 0;
};
} // namespace firebolt::rialto::client::ct

#endif // FIREBOLT_RIALTO_CLIENT_CT_MEDIA_KEYS_TEST_METHODS_H_
