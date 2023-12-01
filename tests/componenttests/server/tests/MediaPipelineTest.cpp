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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Matchers.h"
#include "MediaCommon.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"

using testing::_;
using testing::Return;
using namespace firebolt::rialto::server::ct;

namespace
{
constexpr firebolt::rialto::VideoRequirements kVideoRequirements{1920, 1080};
} // namespace

class MediaPipelineTest : public RialtoServerComponentTest
{
public:
    MediaPipelineTest()
    {
        willConfigureSocket();
        configureSutInActiveState();
        connectClient();
    }
    ~MediaPipelineTest() override = default;

    void createSession()
    {
        // Use matchResponse to store session id
        auto request{createCreateSessionRequest(kVideoRequirements)};
        ConfigureAction<CreateSession>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const ::firebolt::rialto::CreateSessionResponse &resp)
                           { m_sessionId = resp.session_id(); });
    }

    void load()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(_)).WillOnce(Return(nullptr));
        auto request = createLoadRequest(m_sessionId);
        ConfigureAction<Load>(m_clientStub).send(request).expectSuccess();
    }

    int m_sessionId{-1};
};

TEST_F(MediaPipelineTest, shouldCreatePipeline)
{
    createSession();
}

TEST_F(MediaPipelineTest, shouldFailToLoadWhenSessionIdIsWrong)
{
    createSession();
    auto request = createLoadRequest(m_sessionId + 1);
    ConfigureAction<Load>(m_clientStub).send(request).expectFailure();
}

// TEST_F(MediaPipelineTest, shouldLoad)
// {
//     createSession();
//     load();
// }
