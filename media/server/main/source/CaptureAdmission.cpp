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

#include "CaptureAdmission.h"
#include "CaptureConfiguration.h"
#include "CaptureRecordValidator.h"
#include "CaptureSessionGuard.h"
#include "RialtoServerLogging.h"

#include <atomic>
#include <utility>

namespace firebolt::rialto::server
{
namespace
{
class CaptureAdmissionFactory : public ICaptureAdmissionFactory
{
public:
    std::shared_ptr<ICaptureAdmission> createCaptureAdmission() const override
    {
        return std::make_shared<CaptureAdmission>();
    }
};

std::atomic<std::uint32_t> g_nextPoolId{1};

/** Queries the capabilities, retrying "not available yet" until the timeout. */
capture::CaptureStatus queryCapabilities(const capture::CaptureSessionGuard &guard,
                                         capture::CaptureCapabilities &capabilities, const CaptureAdmission::Clock &clock,
                                         const CaptureAdmission::Sleep &sleep, const CaptureSessionHandle &handle)
{
    const std::chrono::milliseconds kTimeout{captureReadyTimeout()};
    const auto kStart{clock()};
    while (true)
    {
        capture::CaptureCapabilities answer{};
        const capture::CaptureStatus kStatus{guard.getCapabilities(answer)};
        if (kStatus != capture::CaptureStatus::CAPABILITIES_NOT_AVAILABLE_YET)
        {
            if (kStatus == capture::CaptureStatus::OK)
            {
                capabilities = answer;
            }
            return kStatus;
        }
        if (clock() - kStart >= kTimeout)
        {
            RIALTO_SERVER_LOG_ERROR("Video frame capture %u of session %d: the capture HAL never became ready at "
                                    "configured READY within %lld ms (a vendor that is ready only at PAUSED fails "
                                    "here)",
                                    handle.captureId, handle.sessionId, static_cast<long long>(kTimeout.count()));
            return kStatus;
        }
        sleep(CaptureAdmission::kRetryInterval);
    }
}
} // namespace

std::shared_ptr<ICaptureAdmissionFactory> &ICaptureAdmissionFactory::instance()
{
    static std::shared_ptr<ICaptureAdmissionFactory> factory{std::make_shared<CaptureAdmissionFactory>()};
    return factory;
}

std::uint32_t nextCapturePoolId()
{
    std::uint32_t id{g_nextPoolId.fetch_add(1)};
    while (id == 0)
    {
        id = g_nextPoolId.fetch_add(1); // wrapped around
    }
    return id;
}

CaptureAdmission::CaptureAdmission(const Clock &clock, const Sleep &sleep) : m_clock{clock}, m_sleep{sleep} {}

bool CaptureAdmission::admit(CaptureSessionHandle &handle)
{
    if (!handle.session)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u of session %d: no HAL session to admit", handle.captureId,
                                handle.sessionId);
        return false;
    }
    capture::CaptureSessionGuard guard{*handle.session};

    // the configured capabilities
    capture::CaptureCapabilities capabilities{};
    const capture::CaptureStatus kQueryStatus{queryCapabilities(guard, capabilities, m_clock, m_sleep, handle)};
    if (kQueryStatus != capture::CaptureStatus::OK)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u of session %d: getCapabilities() failed with status %u",
                                handle.captureId, handle.sessionId, static_cast<unsigned>(kQueryStatus));
        return false;
    }
    const capture::ValidationResult kCapabilitiesCheck{capture::validateCapabilities(capabilities)};
    if (!kCapabilitiesCheck.ok)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u of session %d: invalid capabilities (%s)", handle.captureId,
                                handle.sessionId, kCapabilitiesCheck.rule.c_str());
        return false;
    }

    // the pipeline's maximum video size fits the capture
    if (handle.videoRequirements.maxWidth > capabilities.maximumContentSize.width ||
        handle.videoRequirements.maxHeight > capabilities.maximumContentSize.height)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u of session %d: the pipeline's maximum video size %ux%u exceeds "
                                "the capture's maximum content size %ux%u",
                                handle.captureId, handle.sessionId, handle.videoRequirements.maxWidth,
                                handle.videoRequirements.maxHeight, capabilities.maximumContentSize.width,
                                capabilities.maximumContentSize.height);
        return false;
    }

    // the slot count
    const std::uint32_t kTarget{captureTargetSlots()};
    const std::uint32_t kSlotCount{chooseCaptureSlotCount(capabilities.minimumSlots, capabilities.maximumSlots, kTarget)};
    if (kSlotCount == 0)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u of session %d: the HAL supports at most %u slot, at least 2 "
                                "are needed",
                                handle.captureId, handle.sessionId, capabilities.maximumSlots);
        return false;
    }
    RIALTO_SERVER_LOG_INFO("Video frame capture %u of session %d: minimumSlots %u, maximumSlots %u, target %u, "
                           "admitting %u slots",
                           handle.captureId, handle.sessionId, capabilities.minimumSlots, capabilities.maximumSlots,
                           kTarget, kSlotCount);

    // the pool, created once
    capture::CapturePool pool{};
    const capture::CaptureStatus kPoolStatus{guard.createPool(kSlotCount, pool)};
    if (kPoolStatus != capture::CaptureStatus::OK)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u of session %d: createPool(%u) failed with status %u",
                                handle.captureId, handle.sessionId, kSlotCount, static_cast<unsigned>(kPoolStatus));
        return false;
    }
    const capture::ValidationResult kPoolCheck{capture::validatePool(pool, capabilities, kSlotCount)};
    if (!kPoolCheck.ok)
    {
        RIALTO_SERVER_LOG_ERROR("Video frame capture %u of session %d: invalid pool (%s)", handle.captureId,
                                handle.sessionId, kPoolCheck.rule.c_str());
        return false;
    }

    handle.poolId = nextCapturePoolId();
    handle.pool = std::move(pool);
    handle.capabilities = capabilities;
    RIALTO_SERVER_LOG_MIL("Video frame capture %u of session %d admitted: pool %u with %u slots", handle.captureId,
                          handle.sessionId, handle.poolId, kSlotCount);
    return true;
}
} // namespace firebolt::rialto::server
