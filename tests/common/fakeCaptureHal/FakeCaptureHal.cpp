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

#include "FakeCaptureHalControl.h"
#include "ICaptureSession.h"

#include <sys/mman.h>
#include <unistd.h>

#include <array>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

using namespace firebolt::rialto::capture;

namespace
{
constexpr uint32_t kNv12Fourcc{0x3231564e};
constexpr uint32_t kWidth{1920};
constexpr uint32_t kHeight{1088};
constexpr uint64_t kLumaBytes{static_cast<uint64_t>(kWidth) * kHeight};
constexpr uint64_t kFrameBytes{kLumaBytes + kLumaBytes / 2};

struct FakeState
{
    std::mutex mutex;
    bool factoryReturnsNull{false};
    uint32_t minimumSlots{1};
    uint32_t maximumSlots{4};
    std::array<std::deque<CaptureStatus>, FAKE_CAPTURE_HAL_OPERATION_COUNT> scripted;
    std::array<int, FAKE_CAPTURE_HAL_OPERATION_COUNT> callCounts{};
    int liveSessions{0};
    int destroyedSessions{0};
    bool nextFrameInvalid{false};
};

FakeState &state()
{
    static FakeState fakeState;
    return fakeState;
}

// Counts the call and returns a scripted result if one is queued. Must be called with the state mutex held.
std::optional<CaptureStatus> takeScripted(FakeCaptureHalOperation operation)
{
    FakeState &fake = state();
    ++fake.callCounts[operation];
    if (fake.scripted[operation].empty())
        return std::nullopt;
    const CaptureStatus kStatus = fake.scripted[operation].front();
    fake.scripted[operation].pop_front();
    return kStatus;
}

class FakeCaptureSession final : public ICaptureSession
{
public:
    FakeCaptureSession()
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        ++state().liveSessions;
    }

    ~FakeCaptureSession() override
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        --state().liveSessions;
        ++state().destroyedSessions;
    }

    CaptureStatus getCapabilities(CaptureCapabilities &out) const override
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        if (auto scripted = takeScripted(FAKE_CAPTURE_HAL_GET_CAPABILITIES))
            return *scripted;
        out = capabilities();
        return CaptureStatus::OK;
    }

    CaptureStatus createPool(uint32_t slotCount, CapturePool &outPool) override
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        if (auto scripted = takeScripted(FAKE_CAPTURE_HAL_CREATE_POOL))
            return *scripted;
        if (m_slotCount != 0 || slotCount < state().minimumSlots || slotCount > state().maximumSlots)
            return CaptureStatus::INVALID_ARGUMENT;

        m_slotCount = slotCount;
        m_lockedTokens.assign(slotCount, kInvalidAcquisitionToken);
        outPool.backingSize = Size{kWidth, kHeight};
        outPool.slotLayouts.clear();
        for (uint32_t slot = 0; slot < slotCount; ++slot)
        {
            outPool.slotLayouts.push_back(
                {slot, {{0, 0, 0, kLumaBytes, kWidth}, {1, 0, kLumaBytes, kLumaBytes / 2, kWidth}}});
        }
        return CaptureStatus::OK;
    }

    CaptureStatus acquireCurrentFrame(CapturedFrame &outFrame) override
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        if (auto scripted = takeScripted(FAKE_CAPTURE_HAL_ACQUIRE_CURRENT_FRAME))
            return *scripted;
        if (m_slotCount == 0)
            return CaptureStatus::INVALID_ARGUMENT;

        uint32_t slot{kInvalidCaptureSlotIndex};
        for (uint32_t index = 0; index < m_slotCount; ++index)
        {
            if (m_lockedTokens[index] == kInvalidAcquisitionToken)
            {
                slot = index;
                break;
            }
        }
        if (slot == kInvalidCaptureSlotIndex)
            return CaptureStatus::NO_NEW_FRAME;

        const int kFd = memfd_create("fake-capture-frame", MFD_CLOEXEC);
        if (kFd < 0 || ftruncate(kFd, static_cast<off_t>(kFrameBytes)) != 0)
        {
            if (kFd >= 0)
                close(kFd);
            return CaptureStatus::NO_RESOURCES;
        }

        const uint64_t kObjectId{100 + slot};
        const uint64_t kToken{++m_tokenCounter};
        m_lockedTokens[slot] = kToken;

        outFrame.slotIndex = slot;
        outFrame.acquisitionToken = kToken;
        outFrame.presentationTimeNs = static_cast<int64_t>(kToken) * 40000000;
        outFrame.visibleRegion = Rectangle{0, 0, Size{kWidth, 1080}};
        outFrame.capabilities = capabilities();
        outFrame.colorDescription = {CaptureYcbcrMatrix::BT709, CaptureSampleRange::NARROW,
                                     CaptureChromaSiting::COSITED, CaptureChromaSiting::MIDPOINT};
        outFrame.backingSize = Size{kWidth, kHeight};
        outFrame.dmaBufObjects = {{kObjectId, kFrameBytes}};
        outFrame.planes = {{0, kObjectId, 0, kLumaBytes, kWidth}, {1, kObjectId, kLumaBytes, kLumaBytes / 2, kWidth}};
        outFrame.exportedDmaBufs = {{kObjectId, kFd}};
        if (state().nextFrameInvalid)
        {
            // the chroma plane runs past the end of its object: fails range validation
            state().nextFrameInvalid = false;
            outFrame.planes[1].lengthBytes = kFrameBytes;
        }
        return CaptureStatus::OK;
    }

    CaptureStatus releaseFrame(CapturedFrame &frame) override
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        if (auto scripted = takeScripted(FAKE_CAPTURE_HAL_RELEASE_FRAME))
            return *scripted;
        if (frame.slotIndex >= m_slotCount || frame.acquisitionToken == kInvalidAcquisitionToken ||
            m_lockedTokens[frame.slotIndex] != frame.acquisitionToken)
        {
            return CaptureStatus::INVALID_ARGUMENT;
        }

        m_lockedTokens[frame.slotIndex] = kInvalidAcquisitionToken;
        frame.slotIndex = kInvalidCaptureSlotIndex;
        frame.acquisitionToken = kInvalidAcquisitionToken;
        return CaptureStatus::OK;
    }

private:
    static CaptureCapabilities capabilities()
    {
        CaptureCapabilities caps;
        caps.drmFormat = kNv12Fourcc;
        caps.drmModifier = 0; // DRM_FORMAT_MOD_LINEAR
        caps.maximumContentSize = Size{kWidth, kHeight};
        caps.minimumSlots = state().minimumSlots;
        caps.maximumSlots = state().maximumSlots;
        return caps;
    }

    uint32_t m_slotCount{0};
    std::vector<uint64_t> m_lockedTokens;
    uint64_t m_tokenCounter{0};
};
} // namespace

extern "C"
{
    ICaptureSession *createCaptureSession()
    {
        {
            std::lock_guard<std::mutex> lock{state().mutex};
            if (state().factoryReturnsNull)
                return nullptr;
        }
        return new FakeCaptureSession();
    }

    void fakeCaptureHalReset()
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        FakeState &fake = state();
        fake.factoryReturnsNull = false;
        fake.minimumSlots = 1;
        fake.maximumSlots = 4;
        for (auto &queue : fake.scripted)
            queue.clear();
        fake.callCounts = {};
        fake.destroyedSessions = 0;
        fake.nextFrameInvalid = false;
    }

    void fakeCaptureHalSetFactoryReturnsNull(bool returnsNull)
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        state().factoryReturnsNull = returnsNull;
    }

    void fakeCaptureHalSetSlotLimits(uint32_t minimumSlots, uint32_t maximumSlots)
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        state().minimumSlots = minimumSlots;
        state().maximumSlots = maximumSlots;
    }

    void fakeCaptureHalSetNextFrameInvalid(bool invalid)
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        state().nextFrameInvalid = invalid;
    }

    void fakeCaptureHalPushResult(int operation, uint32_t status)
    {
        if (operation < 0 || operation >= FAKE_CAPTURE_HAL_OPERATION_COUNT)
            return;
        std::lock_guard<std::mutex> lock{state().mutex};
        state().scripted[operation].push_back(static_cast<CaptureStatus>(status));
    }

    int fakeCaptureHalCallCount(int operation)
    {
        if (operation < 0 || operation >= FAKE_CAPTURE_HAL_OPERATION_COUNT)
            return 0;
        std::lock_guard<std::mutex> lock{state().mutex};
        return state().callCounts[operation];
    }

    int fakeCaptureHalLiveSessions()
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        return state().liveSessions;
    }

    int fakeCaptureHalDestroyedSessions()
    {
        std::lock_guard<std::mutex> lock{state().mutex};
        return state().destroyedSessions;
    }
}
