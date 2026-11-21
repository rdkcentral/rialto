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

#ifndef FIREBOLT_RIALTO_CAPTURE_CAPTURE_SESSION_MOCK_H_
#define FIREBOLT_RIALTO_CAPTURE_CAPTURE_SESSION_MOCK_H_

#include "ICaptureSession.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::capture
{
class CaptureSessionMock : public ICaptureSession
{
public:
    MOCK_METHOD(CaptureStatus, getCapabilities, (CaptureCapabilities & out), (const, override));
    MOCK_METHOD(CaptureStatus, createPool, (uint32_t slotCount, CapturePool &outPool), (override));
    MOCK_METHOD(CaptureStatus, acquireCurrentFrame, (CapturedFrame & outFrame), (override));
    MOCK_METHOD(CaptureStatus, releaseFrame, (CapturedFrame & frame), (override));
};
} // namespace firebolt::rialto::capture

#endif // FIREBOLT_RIALTO_CAPTURE_CAPTURE_SESSION_MOCK_H_
