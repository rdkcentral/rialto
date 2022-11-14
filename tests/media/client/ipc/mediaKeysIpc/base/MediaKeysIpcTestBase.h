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

#ifndef MEDIA_KEYS_IPC_TEST_BASE_H_
#define MEDIA_KEYS_IPC_TEST_BASE_H_

#include "EventThreadFactoryMock.h"
#include "EventThreadMock.h"
#include "IpcModuleBase.h"
#include "MediaKeysClientMock.h"
#include "MediaKeysIpc.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::common;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::WithArgs;

MATCHER_P(createMediaKeysRequestMatcher, keySystem, "")
{
    const ::firebolt::rialto::CreateMediaKeysRequest *request =
        dynamic_cast<const ::firebolt::rialto::CreateMediaKeysRequest *>(arg);
    return (request->key_system() == keySystem);
}

MATCHER_P(destroyMediaKeysRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::DestroyMediaKeysRequest *request =
        dynamic_cast<const ::firebolt::rialto::DestroyMediaKeysRequest *>(arg);
    return (request->media_keys_handle() == mediaKeysHandle);
}

class MediaKeysIpcTestBase : public IpcModuleBase, public ::testing::Test
{
public:
    MediaKeysIpcTestBase();
    virtual ~MediaKeysIpcTestBase();

    void setCreateMediaKeysResponse(google::protobuf::Message *response);
    void setCreateKeySessionResponseSuccess(google::protobuf::Message *response);

    static ProtoMediaKeyErrorStatus convertMediaKeyErrorStatus(firebolt::rialto::MediaKeyErrorStatus errorStatus);

protected:
    // MediaKeysIpc object
    std::unique_ptr<IMediaKeys> m_mediaKeysIpc;

    // Strict Mocks
    std::shared_ptr<StrictMock<EventThreadFactoryMock>> m_eventThreadFactoryMock;
    std::unique_ptr<StrictMock<EventThreadMock>> m_eventThread;
    StrictMock<EventThreadMock> *m_eventThreadMock;
    std::shared_ptr<StrictMock<MediaKeysClientMock>> m_mediaKeysClientMock;

    // Common variables
    int32_t m_mediaKeysHandle = 123;
    std::string m_keySystem = "keySystem";
    int32_t m_keySessionId = 456;
    MediaKeyErrorStatus m_errorStatus = MediaKeyErrorStatus::BAD_SESSION_ID;
    std::vector<unsigned char> m_licenseRenewalMessage{'a', 'b', 'c'};

    enum class EventTags
    {
        LicenseRequestEvent = 0,
        LicenseRenewalEvent,
        KeyStatusesChangedEvent
    };

    // Callbacks
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_licenseRequestCb;
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_licenseRenewalCb;
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_KeyStatusesChangeCb;

    void createMediaKeysIpc();
    void destroyMediaKeysIpc();
    void expectSubscribeEvents();
    void expectUnsubscribeEvents();
    void createKeySession();
    std::shared_ptr<firebolt::rialto::LicenseRenewalEvent> createLicenseRenewalEvent();
};

#endif // MEDIA_KEYS_IPC_TEST_BASE_H_
