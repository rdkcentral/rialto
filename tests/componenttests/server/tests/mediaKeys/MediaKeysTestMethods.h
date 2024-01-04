/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_CT_MEDIA_KEYS_TEST_METHODS_H_
#define FIREBOLT_RIALTO_SERVER_CT_MEDIA_KEYS_TEST_METHODS_H_

#include <memory>
#include <vector>

#include "MessageBuilders.h"
#include "OcdmSessionMock.h"
#include "RialtoServerComponentTest.h"

namespace firebolt::rialto::server::ct
{
class MediaKeysTestMethods : public RialtoServerComponentTest
{
public:
    MediaKeysTestMethods();
    ~MediaKeysTestMethods() override = default;

    void createMediaKeysWidevine();
    void createMediaKeysNetflix();

    void createKeySession();
    void ocdmSessionWillBeCreated();

    void shouldFailToCreateSessionWhenMksIdIsWrong();

    void generateRequest();

    void updateSessionWidevine();
    void updateSessionNetflix();
    void updateAllKeys();
    void updateOneKey();
    void licenseRenewal();
    void containsKey();
    void doesNotContainKey();
    void removeKeySession();
    void loadKeySession();
    void closeKeySessionWidevine();
    void closeKeySessionNetflix();
    void setDrmHeader();
    void setDrmHeader2();
    void getLastDrmError();
    void getCdmKeySessionId();
    void destroyMediaKeysRequest();
    void deleteDrmStoreRequest();
    void deleteKeyStoreRequest();
    void getDrmStoreHashRequest();
    void getDrmStoreHashRequestFails();
    void getKeyStoreHashRequest();
    void getKeyStoreHashRequestFails();
    void getLdlSessionsLimitRequest();
    void getDrmTimeRequest();

private:
    void setDrmHeader(const std::vector<unsigned char> &kKeyId);
    void createMediaKeys(const ::firebolt::rialto::CreateMediaKeysRequest &request);

    int m_mediaKeysHandle{-1};
    int m_mediaKeySessionId{-1};
    std::unique_ptr<testing::StrictMock<wrappers::OcdmSessionMock>> m_ocdmSession{
        std::make_unique<testing::StrictMock<wrappers::OcdmSessionMock>>()};
    testing::StrictMock<wrappers::OcdmSessionMock> &m_ocdmSessionMock{*m_ocdmSession};
    firebolt::rialto::wrappers::IOcdmSessionClient *m_ocdmSessionClient{0};
    std::mutex m_mutex;
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_MEDIA_KEYS_TEST_METHODS_H_
