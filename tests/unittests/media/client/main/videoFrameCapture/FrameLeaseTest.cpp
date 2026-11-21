/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "FrameLease.h"
#include "VideoFrameCaptureIpcMock.h"

#include <gtest/gtest.h>

#include <functional>
#include <memory>

using namespace firebolt::rialto::client;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{
/** An importer that keeps or declines a deferred release (R13 design D5). */
class DeferringImporter : public IFrameImporter
{
public:
    explicit DeferringImporter(bool defer) : m_defer{defer} {}
    std::shared_ptr<IImportedImage> importFrame(const CaptureFrameDescription &, const std::vector<CaptureObjectFd> &,
                                                bool &) override
    {
        return nullptr;
    }
    void retire(const std::shared_ptr<IImportedImage> &) override {}
    void drain() override {}
    bool deferRelease(const std::shared_ptr<IImportedImage> &image, std::function<void()> sendRelease) override
    {
        ++m_offers;
        m_image = image;
        if (m_defer)
            m_sendRelease = std::move(sendRelease);
        return m_defer;
    }

    const bool m_defer;
    int m_offers{0};
    std::shared_ptr<IImportedImage> m_image;
    std::function<void()> m_sendRelease;
};

class FakeImage : public IImportedImage
{
public:
    std::uint32_t textureTarget() const override { return 0; }
    const std::vector<std::uint32_t> &textureIds() const override { return m_ids; }

private:
    std::vector<std::uint32_t> m_ids;
};
} // namespace

class FrameLeaseTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<VideoFrameCaptureIpcMock>> m_ipc{std::make_shared<StrictMock<VideoFrameCaptureIpcMock>>()};
    std::shared_ptr<CaptureLeaseState> m_state{std::make_shared<CaptureLeaseState>(m_ipc, 5)};
    std::shared_ptr<IImportedImage> m_image{std::make_shared<FakeImage>()};
};

TEST_F(FrameLeaseTest, ReleasesAtOnceWithoutAnImporter)
{
    auto lease = std::make_unique<FrameLease>(m_state, 1, 2, 3);
    EXPECT_CALL(*m_ipc, releaseFrame(5, 1, 2, 3)).WillOnce(Return(true));
    lease.reset();
}

TEST_F(FrameLeaseTest, ReleasesAtOnceWhenTheImporterDeclines)
{
    auto importer = std::make_shared<DeferringImporter>(false);
    auto lease = std::make_unique<FrameLease>(m_state, 1, 2, 3);
    lease->deferTo(importer, m_image);
    EXPECT_CALL(*m_ipc, releaseFrame(5, 1, 2, 3)).WillOnce(Return(true));
    lease.reset();
    EXPECT_EQ(importer->m_offers, 1);
    EXPECT_EQ(importer->m_image, m_image);
}

TEST_F(FrameLeaseTest, ReleasesOnlyWhenTheImporterSendsIt)
{
    auto importer = std::make_shared<DeferringImporter>(true);
    auto lease = std::make_unique<FrameLease>(m_state, 1, 2, 3);
    lease->deferTo(importer, m_image);
    lease.reset(); // the strict IPC mock fails the test if the release goes now
    ASSERT_TRUE(importer->m_sendRelease);
    EXPECT_CALL(*m_ipc, releaseFrame(5, 1, 2, 3)).WillOnce(Return(true));
    importer->m_sendRelease();
}
