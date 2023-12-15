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

#include "MediaKeysModuleStub.h"
#include "MediaKeysProtoUtils.h"
#include <IIpcServer.h>
#include <IIpcServerFactory.h>
#include <gtest/gtest.h>
#include <memory>

namespace firebolt::rialto::client::ct
{
MediaKeysModuleStub::MediaKeysModuleStub(const std::shared_ptr<::firebolt::rialto::MediaKeysModule> &mediaKeysModuleMock)
    : m_mediaKeysModuleMock{mediaKeysModuleMock}
{
}

MediaKeysModuleStub::~MediaKeysModuleStub() {}

void MediaKeysModuleStub::notifyLicenseRequestEvent(int32_t mediaKeysHandle, int32_t keySessionId,
                                                    const std::vector<unsigned char> &licenseRequestMessage,
                                                    const std::string &url)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::LicenseRequestEvent>();
    for (auto it = licenseRequestMessage.begin(); it != licenseRequestMessage.end(); it++)
    {
        event->add_license_request_message(*it);
    }
    event->set_media_keys_handle(mediaKeysHandle);
    event->set_key_session_id(keySessionId);
    event->set_url(url);
    getClient()->sendEvent(event);
}

void MediaKeysModuleStub::notifyKeyStatusesChangedEvent(int32_t mediaKeysHandle, int32_t keySessionId,
                                                        const KeyStatusVector &keyStatuses)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::KeyStatusesChangedEvent>();
    for (auto it = keyStatuses.begin(); it != keyStatuses.end(); it++)
    {
        ::firebolt::rialto::KeyStatusesChangedEvent_KeyStatusPair *keyStatusPair = event->add_key_statuses();
        for (auto it2 = it->first.begin(); it2 != it->first.end(); it2++)
        {
            keyStatusPair->add_key_id(*it2);
        }
        keyStatusPair->set_key_status(convertKeyStatus(it->second));
    }
    event->set_media_keys_handle(mediaKeysHandle);
    event->set_key_session_id(keySessionId);

    getClient()->sendEvent(event);
}

} // namespace firebolt::rialto::client::ct
