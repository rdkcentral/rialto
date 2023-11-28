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

#ifndef MEDIA_PIPELINE_TEST_METHODS_H_
#define MEDIA_PIPELINE_TEST_METHODS_H_

#include "MediaPipelineClientMock.h"
#include "MediaPipelineModuleMock.h"
#include "ServerStub.h"
#include "IMediaPipeline.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::componenttest::stub;

class MediaPipelineTestMethods
{
public:
    MediaPipelineTestMethods();
    virtual ~MediaPipelineTestMethods();

protected:
    // Strict Mocks
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClientMock;
    std::shared_ptr<StrictMock<MediaPipelineModuleMock>> m_mediaPipelineModuleMock;

    // Objects
    std::shared_ptr<IMediaPipelineFactory> m_mediaPipelineFactory;
    std::unique_ptr<IMediaPipeline> m_mediaPipeline;

    // Test methods
    void createMediaPipeline();

    // Component test helpers
    virtual void notifyEvent() = 0;
    virtual void waitEvent() = 0;
    virtual std::shared_ptr<ServerStub>& getServerStub() = 0;

private:
};

#endif // MEDIA_PIPELINE_TEST_METHODS_H_
