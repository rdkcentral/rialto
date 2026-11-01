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

#ifndef FIREBOLT_RIALTO_SERVER_ACTIVE_REQUESTS_H_
#define FIREBOLT_RIALTO_SERVER_ACTIVE_REQUESTS_H_

#include "IActiveRequests.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace firebolt::rialto::server
{
class ActiveRequestsData : public std::enable_shared_from_this<ActiveRequestsData>
{
public:
    ActiveRequestsData(MediaSourceType type, std::uint32_t maxMediaBytes, std::uint32_t maxFrames)
        : m_type(type), m_bytesWritten(0), m_maxMediaBytes(maxMediaBytes), m_maxFrames(maxFrames)
    {
    }
    ~ActiveRequestsData();
    ActiveRequestsData(ActiveRequestsData &&) = delete;
    ActiveRequestsData &operator=(ActiveRequestsData &&) = delete;

    ActiveRequestsData(const ActiveRequestsData &) = delete;
    ActiveRequestsData &operator=(const ActiveRequestsData &) = delete;

    AddSegmentStatus addSegment(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment);
    void withSegments(const std::function<void(const IMediaPipeline::MediaSegmentVector &)> &func);
    // Returns a lifetime-pinning view of the segment vector. Callers must not read it concurrently
    // with addSegment() on the same request - the protocol guarantees no addSegment after haveData.
    std::shared_ptr<const IMediaPipeline::MediaSegmentVector> segments();

    MediaSourceType getType() const { return m_type; }
    std::uint32_t getMaxFrames() const { return m_maxFrames; }

private:
    MediaSourceType m_type;
    std::uint32_t m_bytesWritten;
    std::uint32_t m_maxMediaBytes;
    std::uint32_t m_maxFrames;
    IMediaPipeline::MediaSegmentVector m_segments;
    std::vector<std::vector<uint8_t>> m_segmentBuffers;
    std::mutex m_dataMutex;
};

class ActiveRequests : public IActiveRequests
{
public:
    ActiveRequests();
    ~ActiveRequests() override = default;
    ActiveRequests(const ActiveRequests &) = delete;
    ActiveRequests(ActiveRequests &&) = delete;
    ActiveRequests &operator=(const ActiveRequests &) = delete;
    ActiveRequests &operator=(ActiveRequests &&) = delete;

    std::uint32_t insert(const MediaSourceType &mediaSourceType, std::uint32_t maxMediaBytes,
                         std::uint32_t maxFrames) override;
    MediaSourceType getType(std::uint32_t requestId) const override;
    std::uint32_t getMaxFrames(std::uint32_t requestId) const override;
    void erase(std::uint32_t requestId) override;
    void erase(const MediaSourceType &mediaSourceType) override;
    void clear() override;
    AddSegmentStatus addSegment(std::uint32_t requestId,
                                const std::unique_ptr<IMediaPipeline::MediaSegment> &segment) override;
    std::shared_ptr<ActiveRequestsData> getRequest(std::uint32_t requestId) const override;

private:
    mutable std::mutex m_mutex;
    std::uint32_t m_currentId;
    std::map<std::uint32_t, std::shared_ptr<ActiveRequestsData>> m_requestMap;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_ACTIVE_REQUESTS_H_
