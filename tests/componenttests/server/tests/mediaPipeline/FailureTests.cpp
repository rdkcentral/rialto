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
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

namespace firebolt::rialto::server::ct
{
/*
 * Component Test: Load failure sequence
 * Test Objective:
 *  Test the gstreamer pipeline is not created when LoadRequest with invalid session id is sent
 *
 * Sequence Diagrams:
 *  Create, Destroy - https://wiki.rdkcentral.com/pages/viewpage.action?pageId=226375556
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  Map Shared Memory
 *
 * Test Steps:
 *  Step 1: Create a new media session
 *   Send CreateSessionRequest to Rialto Server
 *   Expect that successful CreateSessionResponse is received
 *   Save returned session id
 *
 *  Step 2: Fail to load content
 *   Send LoadRequest to Rialto Server with invalid session id
 *   Expect that failure LoadResponse is received
 *   Expect that GstPlayer instance is not created.
 *
 *  Step 3: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  Failure API calls are handled by the server.
 *
 * Code:
 */
TEST_F(MediaPipelineTest, shouldFailToLoadWhenSessionIdIsWrong)
{
    // Step 1: Create a new media session
    createSession();

    // Step 2: Fail to load content
    auto request = createLoadRequest(m_sessionId + 1);
    ConfigureAction<Load>(m_clientStub).send(request).expectFailure();

    // Step 3: Destroy media session
    destroySession();
}
} // namespace firebolt::rialto::server::ct
