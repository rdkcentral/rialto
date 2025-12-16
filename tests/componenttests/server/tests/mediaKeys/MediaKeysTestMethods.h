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
#include <string>
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

    void willGenerateRequestPlayready();
    void generateRequestPlayready();

    void willUpdateSessionNetflix();
    void updateSessionNetflix();

    void willCloseKeySessionPlayready();
    void closeKeySessionPlayready();

    void willTeardown();
    void willRelease();

private:
    void createMediaKeys(const ::firebolt::rialto::CreateMediaKeysRequest &request);

protected:
    int m_mediaKeysHandle{-1};
    int m_mediaKeySessionId{-1};
    std::unique_ptr<testing::StrictMock<wrappers::OcdmSessionMock>> m_ocdmSession{
        std::make_unique<testing::StrictMock<wrappers::OcdmSessionMock>>()};
    testing::StrictMock<wrappers::OcdmSessionMock> &m_ocdmSessionMock{*m_ocdmSession};
    firebolt::rialto::wrappers::IOcdmSessionClient *m_ocdmSessionClient{0};
    
    const std::vector<unsigned char> m_kUpdateSessionNetflixResponse{5, 6};
    const std::vector<unsigned char> m_kInitData{1, 2, 7};
    const std::vector<uint8_t> m_kLicenseRequestMessage{'d', 'z', 'f'};
};

} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_MEDIA_KEYS_TEST_METHODS_H_
