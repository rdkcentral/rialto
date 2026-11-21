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

#include "CaptureSessionGuard.h"
#include "RialtoLogging.h"

#include <exception>

namespace firebolt::rialto::capture
{
namespace
{
template <typename Call> CaptureStatus guarded(const char *operation, Call call)
{
    try
    {
        return call();
    }
    catch (const std::exception &exception)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_SERVER, "Capture HAL %s threw an exception: %s", operation, exception.what());
    }
    catch (...)
    {
        RIALTO_LOG_ERROR(RIALTO_COMPONENT_SERVER, "Capture HAL %s threw an exception", operation);
    }
    return CaptureStatus::FATAL_ERROR;
}
} // namespace

CaptureSessionGuard::CaptureSessionGuard(ICaptureSession &session) : m_session{session} {}

CaptureStatus CaptureSessionGuard::getCapabilities(CaptureCapabilities &out) const
{
    return guarded("getCapabilities", [&]() { return m_session.getCapabilities(out); });
}

CaptureStatus CaptureSessionGuard::createPool(uint32_t slotCount, CapturePool &outPool)
{
    return guarded("createPool", [&]() { return m_session.createPool(slotCount, outPool); });
}

CaptureStatus CaptureSessionGuard::acquireCurrentFrame(CapturedFrame &outFrame)
{
    return guarded("acquireCurrentFrame", [&]() { return m_session.acquireCurrentFrame(outFrame); });
}

CaptureStatus CaptureSessionGuard::releaseFrame(CapturedFrame &frame)
{
    return guarded("releaseFrame", [&]() { return m_session.releaseFrame(frame); });
}

} // namespace firebolt::rialto::capture
