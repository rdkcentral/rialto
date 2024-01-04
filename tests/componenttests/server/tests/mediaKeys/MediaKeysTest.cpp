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

#include <vector>

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Matchers.h"
#include "MediaKeysTestMethods.h"

using testing::_;
using testing::ByMove;
using testing::Return;
using testing::StrictMock;

namespace firebolt::rialto::server::ct
{
class MediaKeysTest : public MediaKeysTestMethods
{
public:
    MediaKeysTest() {}
    virtual ~MediaKeysTest() {}
};

TEST_F(MediaKeysTest, shouldFailToCreateSessionWhenMksIdIsWrong)
{
    createMediaKeysWidevine();
    shouldFailToCreateSessionWhenMksIdIsWrong();
}

/*
 * Component Test:
 * Test Objective:
 *
 *
 * Sequence Diagrams:
 *
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: RialtoApplicationSessionServer with stubs for RialtoClient and RialtoServerManager
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts the application server running in its own thread
 *
 *
 * Test Steps:
 *  Step A1:
 *
 *
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *
 * Code:
 */
TEST_F(MediaKeysTest, shouldGenerate)
{
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();
    createKeySession();
    generateRequest();
}

} // namespace firebolt::rialto::server::ct
