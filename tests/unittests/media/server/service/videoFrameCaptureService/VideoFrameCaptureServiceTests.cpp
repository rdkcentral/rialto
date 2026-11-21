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

#include "CaptureHalLibraryFactoryMock.h"
#include "CaptureHalLibraryMock.h"
#include "CaptureSessionMock.h"
#include "VideoFrameCaptureRegistry.h"
#include "VideoFrameCaptureService.h"

#include <gtest/gtest.h>

#include <set>
#include <utility>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server::service;
using firebolt::rialto::wrappers::CaptureHalLibraryFactoryMock;
using firebolt::rialto::wrappers::CaptureHalLibraryMock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{
constexpr VideoFrameCaptureClientId kClientA{1};
constexpr VideoFrameCaptureClientId kClientB{2};

// A loader factory whose library is available, or not.
std::shared_ptr<CaptureHalLibraryFactoryMock> halFactory(bool available)
{
    auto library = std::make_shared<NiceMock<CaptureHalLibraryMock>>();
    ON_CALL(*library, isAvailable()).WillByDefault(Return(available));
    auto factory = std::make_shared<NiceMock<CaptureHalLibraryFactoryMock>>();
    ON_CALL(*factory, getCaptureHalLibrary()).WillByDefault(Return(library));
    return factory;
}
} // namespace

TEST(VideoFrameCaptureRegistryTest, ShouldGiveUniqueNonZeroIdsAndNeverReuseThem)
{
    VideoFrameCaptureRegistry registry;
    const std::uint32_t kFirst = registry.add(GraphicsAPIType::GL);
    const std::uint32_t kSecond = registry.add(GraphicsAPIType::VULKAN);
    EXPECT_NE(kFirst, 0U);
    EXPECT_NE(kSecond, 0U);
    EXPECT_NE(kFirst, kSecond);

    EXPECT_TRUE(registry.remove(kSecond));
    const std::uint32_t kThird = registry.add(GraphicsAPIType::GL);
    EXPECT_NE(kThird, kFirst);
    EXPECT_NE(kThird, kSecond); // a destroyed ID is not handed out again

    ASSERT_NE(registry.find(kFirst), nullptr);
    EXPECT_EQ(registry.find(kFirst)->api, GraphicsAPIType::GL);
    EXPECT_EQ(registry.find(kSecond), nullptr);
    EXPECT_FALSE(registry.remove(kSecond));
    EXPECT_EQ(registry.size(), 2U);
}

TEST(VideoFrameCaptureServiceTest, ShouldRegisterCapturesPerClient)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::uint32_t first{0}, second{0};
    EXPECT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, first), VideoFrameCaptureServiceStatus::OK);
    EXPECT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, second), VideoFrameCaptureServiceStatus::OK);
    EXPECT_NE(first, 0U);
    EXPECT_NE(first, second);
    EXPECT_TRUE(service.hasCapture(kClientA, first));
    EXPECT_TRUE(service.hasCapture(kClientA, second));
}

TEST(VideoFrameCaptureServiceTest, ShouldNotFindCaptureOfAnotherClient)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::uint32_t captureId{0};
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, captureId), VideoFrameCaptureServiceStatus::OK);

    EXPECT_FALSE(service.hasCapture(kClientB, captureId));
    EXPECT_FALSE(service.destroyCapture(kClientB, captureId));
    EXPECT_TRUE(service.hasCapture(kClientA, captureId)); // unaffected
}

TEST(VideoFrameCaptureServiceTest, ShouldDestroyOnceAndNotFindDestroyedId)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::uint32_t captureId{0};
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, captureId), VideoFrameCaptureServiceStatus::OK);

    EXPECT_TRUE(service.destroyCapture(kClientA, captureId));
    EXPECT_FALSE(service.hasCapture(kClientA, captureId));
    EXPECT_FALSE(service.destroyCapture(kClientA, captureId));
    EXPECT_FALSE(service.hasCapture(kClientA, 12345));
}

TEST(VideoFrameCaptureServiceTest, ShouldRemoveOnlyTheDisconnectedClientsCaptures)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::uint32_t a1{0}, a2{0}, b1{0};
    service.createCapture(kClientA, GraphicsAPIType::GL, a1);
    service.createCapture(kClientA, GraphicsAPIType::GL, a2);
    service.createCapture(kClientB, GraphicsAPIType::GL, b1);

    service.removeClient(kClientA);

    EXPECT_FALSE(service.hasCapture(kClientA, a1));
    EXPECT_FALSE(service.hasCapture(kClientA, a2));
    EXPECT_TRUE(service.hasCapture(kClientB, b1));
}

TEST(VideoFrameCaptureServiceTest, ShouldReturnUnsupportedWithoutCaptureHal)
{
    VideoFrameCaptureService service{halFactory(false)};
    std::uint32_t captureId{0};
    EXPECT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, captureId),
              VideoFrameCaptureServiceStatus::UNSUPPORTED);
    EXPECT_EQ(captureId, 0U);
    EXPECT_FALSE(service.hasCapture(kClientA, 1));
}

TEST(VideoFrameCaptureServiceTest, ShouldReturnUnsupportedWithoutLoader)
{
    auto factory = std::make_shared<StrictMock<CaptureHalLibraryFactoryMock>>();
    EXPECT_CALL(*factory, getCaptureHalLibrary()).WillRepeatedly(Return(nullptr));
    VideoFrameCaptureService service{factory};
    std::uint32_t captureId{0};
    EXPECT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, captureId),
              VideoFrameCaptureServiceStatus::UNSUPPORTED);
}

TEST(VideoFrameCaptureServiceTest, ShouldRejectUnknownGraphicsApi)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::uint32_t captureId{0};
    EXPECT_EQ(service.createCapture(kClientA, std::nullopt, captureId), VideoFrameCaptureServiceStatus::INVALID_ARGUMENT);
    EXPECT_FALSE(service.hasCapture(kClientA, 1));
}

TEST(VideoFrameCaptureServiceTest, ShouldGetTheLoaderOnce)
{
    auto library = std::make_shared<StrictMock<CaptureHalLibraryMock>>();
    EXPECT_CALL(*library, isAvailable()).Times(2).WillRepeatedly(Return(true));
    auto factory = std::make_shared<StrictMock<CaptureHalLibraryFactoryMock>>();
    EXPECT_CALL(*factory, getCaptureHalLibrary()).WillOnce(Return(library));

    VideoFrameCaptureService service{factory};
    std::uint32_t first{0}, second{0};
    EXPECT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, first), VideoFrameCaptureServiceStatus::OK);
    EXPECT_EQ(service.createCapture(kClientB, GraphicsAPIType::VULKAN, second), VideoFrameCaptureServiceStatus::OK);
}

TEST(VideoFrameCaptureServiceTest, ShouldBindACaptureToOneSessionAtATime)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::uint32_t captureId{0};
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, captureId), VideoFrameCaptureServiceStatus::OK);

    EXPECT_TRUE(service.bindCapture(kClientA, captureId, 10));
    EXPECT_FALSE(service.bindCapture(kClientA, captureId, 11)); // already bound elsewhere
    EXPECT_FALSE(service.bindCapture(kClientA, captureId, 10)); // or to the same session
    service.unbindCapture(kClientA, captureId);
    EXPECT_TRUE(service.bindCapture(kClientA, captureId, 11)); // reusable after unbinding
}

TEST(VideoFrameCaptureServiceTest, ShouldNotBindAnotherClientsOrUnknownCapture)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::uint32_t captureId{0};
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, captureId), VideoFrameCaptureServiceStatus::OK);

    EXPECT_FALSE(service.bindCapture(kClientB, captureId, 10));
    EXPECT_FALSE(service.bindCapture(kClientA, captureId + 1, 10));
    service.unbindCapture(kClientB, captureId); // harmless
    EXPECT_TRUE(service.bindCapture(kClientA, captureId, 10));
}

TEST(VideoFrameCaptureServiceTest, ShouldReportDestroyedBoundCaptureOnly)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::vector<std::pair<int, std::uint32_t>> removed;
    service.setBoundCaptureRemovedHandler([&removed](int sessionId, std::uint32_t captureId)
                                          { removed.emplace_back(sessionId, captureId); });
    std::uint32_t bound{0}, unbound{0};
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, bound), VideoFrameCaptureServiceStatus::OK);
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, unbound), VideoFrameCaptureServiceStatus::OK);
    ASSERT_TRUE(service.bindCapture(kClientA, bound, 10));

    EXPECT_TRUE(service.destroyCapture(kClientA, unbound));
    EXPECT_TRUE(removed.empty());
    EXPECT_TRUE(service.destroyCapture(kClientA, bound));
    ASSERT_EQ(removed.size(), 1U);
    EXPECT_EQ(removed[0], std::make_pair(10, bound));
}

TEST(VideoFrameCaptureServiceTest, ShouldReportBoundCapturesOfDisconnectedClient)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::vector<std::pair<int, std::uint32_t>> removed;
    service.setBoundCaptureRemovedHandler([&removed](int sessionId, std::uint32_t captureId)
                                          { removed.emplace_back(sessionId, captureId); });
    std::uint32_t first{0}, second{0}, other{0};
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, first), VideoFrameCaptureServiceStatus::OK);
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, second), VideoFrameCaptureServiceStatus::OK);
    ASSERT_EQ(service.createCapture(kClientB, GraphicsAPIType::GL, other), VideoFrameCaptureServiceStatus::OK);
    ASSERT_TRUE(service.bindCapture(kClientA, first, 10));
    ASSERT_TRUE(service.bindCapture(kClientB, other, 11));

    service.removeClient(kClientA);
    ASSERT_EQ(removed.size(), 1U); // only client A's bound capture
    EXPECT_EQ(removed[0], std::make_pair(10, first));
}

TEST(VideoFrameCaptureServiceTest, ShouldGiveTheRuntimeOnlyToItsOwnClient)
{
    VideoFrameCaptureService service{halFactory(true)};
    std::uint32_t captureId{0};
    ASSERT_EQ(service.createCapture(kClientA, GraphicsAPIType::GL, captureId), VideoFrameCaptureServiceStatus::OK);
    EXPECT_EQ(service.captureRuntime(kClientA, captureId), nullptr); // not committed yet

    auto runtime = std::make_shared<firebolt::rialto::server::CaptureSessionRuntime>(
        firebolt::rialto::wrappers::CaptureSessionPtr{new ::testing::NiceMock<firebolt::rialto::capture::CaptureSessionMock>(),
                                                      [](firebolt::rialto::capture::ICaptureSession *session)
                                                      { delete session; }},
        1, firebolt::rialto::capture::CapturePool{});
    service.setCaptureRuntime(kClientA, captureId, runtime);
    EXPECT_EQ(service.captureRuntime(kClientA, captureId), runtime);
    EXPECT_EQ(service.captureRuntime(kClientB, captureId), nullptr);
    service.setCaptureRuntime(kClientB, captureId, runtime); // another client's: ignored
    EXPECT_TRUE(service.destroyCapture(kClientA, captureId));
    EXPECT_EQ(service.captureRuntime(kClientA, captureId), nullptr);
}
