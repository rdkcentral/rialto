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

#ifndef FIREBOLT_RIALTO_CAPTURE_CAPTURE_SESSION_GUARD_H_
#define FIREBOLT_RIALTO_CAPTURE_CAPTURE_SESSION_GUARD_H_

#include "ICaptureSession.h"

namespace firebolt::rialto::capture
{
/**
 * @brief The only way Rialto calls a capture HAL session.
 *
 * SoC capture spec 8.1 forbids exceptions crossing the HAL boundary. If a vendor call throws anyway, the guard logs
 * the operation and reports FATAL_ERROR instead of letting the exception terminate the server.
 */
class CaptureSessionGuard
{
public:
    explicit CaptureSessionGuard(ICaptureSession &session);

    CaptureStatus getCapabilities(CaptureCapabilities &out) const;
    CaptureStatus createPool(uint32_t slotCount, CapturePool &outPool);
    CaptureStatus acquireCurrentFrame(CapturedFrame &outFrame);
    CaptureStatus releaseFrame(CapturedFrame &frame);

private:
    ICaptureSession &m_session;
};

} // namespace firebolt::rialto::capture

#endif // FIREBOLT_RIALTO_CAPTURE_CAPTURE_SESSION_GUARD_H_
