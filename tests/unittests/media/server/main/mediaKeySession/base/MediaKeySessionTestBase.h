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

#ifndef MEDIA_KEY_SESSION_TEST_BASE_H_
#define MEDIA_KEY_SESSION_TEST_BASE_H_

#include "MainThreadFactoryMock.h"
#include "MainThreadMock.h"
#include "MediaKeySession.h"
#include "MediaKeysClientMock.h"
#include "MediaKeysCommon.h"
#include "OcdmSessionMock.h"
#include "OcdmSystemMock.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;
using namespace firebolt::rialto::server::mock;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

class MediaKeySessionTestBase : public ::testing::Test
{
public:
    MediaKeySessionTestBase();
    virtual ~MediaKeySessionTestBase();

protected:
    // MediaKeys object
    std::unique_ptr<MediaKeySession> m_mediaKeySession;

    // Strict Mocks
    std::shared_ptr<StrictMock<MediaKeysClientMock>> m_mediaKeysClientMock;
    std::unique_ptr<StrictMock<OcdmSystemMock>> m_ocdmSystemMock;
    std::unique_ptr<StrictMock<OcdmSessionMock>> m_ocdmSession;
    StrictMock<OcdmSessionMock> *m_ocdmSessionMock;
    std::shared_ptr<StrictMock<MainThreadFactoryMock>> m_mainThreadFactoryMock;
    std::shared_ptr<StrictMock<MainThreadMock>> m_mainThreadMock;

    // Common variables
    const int32_t m_kKeySessionId = 123;
    const int32_t m_kMainThreadClientId = {5};
    KeySessionType m_keySessionType = KeySessionType::PERSISTENT_RELEASE_MESSAGE;
    bool m_isLDL = false;

    void createKeySession(const std::string &keySystem);
    void destroyKeySession();
    void expectCloseKeySession(const std::string &keySystem);
    void generateRequest();
    void mainThreadWillEnqueueTask();
};

#endif // MEDIA_KEY_SESSION_TEST_BASE_H_
