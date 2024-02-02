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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"

using ::testing::UnorderedElementsAre;

namespace firebolt::rialto::server::ct
{
class MediaPipelineCapabilitiesTest : public RialtoServerComponentTest
{
public:
    MediaPipelineCapabilitiesTest()
    {
        willConfigureSocket();
        configureSutInActiveState();
        connectClient();
    }
    ~MediaPipelineCapabilitiesTest() override = default;
};

TEST_F(MediaPipelineCapabilitiesTest, getAudioCapabilities)
{
    auto request{createGetSupportedMimeTypesRequest(MediaSourceType::AUDIO)};
    ConfigureAction<GetSupportedMimeTypes>{m_clientStub}.send(request).expectSuccess().matchResponse(
        [](const auto &resp)
        {
            // NOTE: Supported Mime types are initialized during SUT initialization, in RialtoServerComponentTest::startSut()
            // Returned mime types should intersect with "audio/mpeg, mpegversion=(int)4" static pad caps
            EXPECT_THAT(resp.mime_types(), UnorderedElementsAre("audio/x-eac3", "audio/aac", "audio/mp4"));
        });
};
} // namespace firebolt::rialto::server::ct
