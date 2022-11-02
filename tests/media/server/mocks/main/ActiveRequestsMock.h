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

#ifndef FIREBOLT_RIALTO_SERVER_MOCK_ACTIVE_REQUESTS_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_MOCK_ACTIVE_REQUESTS_MOCK_H_

#include "IActiveRequests.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::server::mock
{
class ActiveRequestsMock : public IActiveRequests
{
public:
    MOCK_METHOD(std::uint32_t, insert, (const MediaSourceType &mediaSourceType, std::uint32_t maxMediaBytes), (override));
    MOCK_METHOD(MediaSourceType, getType, (std::uint32_t requestId), (const, override));
    MOCK_METHOD(void, erase, (std::uint32_t requestId), (override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(AddSegmentStatus, addSegment,
                (std::uint32_t requestId, const std::unique_ptr<IMediaPipeline::MediaSegment> &segment), (override));
    MOCK_METHOD(const IMediaPipeline::MediaSegmentVector &, getSegments, (std::uint32_t requestId), (const, override));
};
} // namespace firebolt::rialto::server::mock

#endif // FIREBOLT_RIALTO_SERVER_MOCK_ACTIVE_REQUESTS_MOCK_H_
