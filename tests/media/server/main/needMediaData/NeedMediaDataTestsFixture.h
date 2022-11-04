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

#ifndef NEED_MEDIA_DATA_TESTS_FIXTURE_H_
#define NEED_MEDIA_DATA_TESTS_FIXTURE_H_

#include "ActiveRequestsMock.h"
#include "MediaPipelineClientMock.h"
#include "NeedMediaData.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class NeedMediaDataTests : public testing::Test
{
public:
    NeedMediaDataTests();
    ~NeedMediaDataTests() override = default;

    void initialize();
    void initializeWithWrongType();

    void needMediaDataWillBeSent();
    void needMediaDataWillNotBeSent();

private:
    std::unique_ptr<firebolt::rialto::server::NeedMediaData> m_sut;
    std::shared_ptr<StrictMock<firebolt::rialto::MediaPipelineClientMock>> m_clientMock;
    StrictMock<firebolt::rialto::server::ActiveRequestsMock> activeRequestsMock;
    StrictMock<firebolt::rialto::server::SharedMemoryBufferMock> shmBufferMock;
};

#endif // NEED_MEDIA_DATA_TESTS_FIXTURE_H_
