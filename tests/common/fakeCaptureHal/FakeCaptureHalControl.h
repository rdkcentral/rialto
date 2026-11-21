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

#ifndef FIREBOLT_RIALTO_CAPTURE_FAKE_CAPTURE_HAL_CONTROL_H_
#define FIREBOLT_RIALTO_CAPTURE_FAKE_CAPTURE_HAL_CONTROL_H_

/**
 * @file FakeCaptureHalControl.h
 *
 * Control API of the fake capture HAL library (libfake_capture_hal.so), used in tests in place of a vendor
 * lib_capture_hal.so. Tests load the library with dlopen and resolve these functions with dlsym, so the library
 * can also be unloaded. All functions are thread safe.
 *
 * Default behaviour:
 *  - getCapabilities(): OK with the configured capabilities (NV12 linear, 1920x1088, slots [1, 4]);
 *  - createPool(n): OK for n within the limits, INVALID_ARGUMENT otherwise;
 *  - acquireCurrentFrame(): OK with an NV12 frame in one DMA-BUF object (SoC spec example B), a real memfd as its
 *    exported descriptor, the next free slot and a new non-zero token; NO_NEW_FRAME when every slot is locked;
 *  - releaseFrame(): OK for the matching slot and token, which it then invalidates; INVALID_ARGUMENT otherwise.
 * A scripted result, if queued for an operation, replaces the default for one call and leaves the output unchanged.
 */

#include <cstdint>

extern "C"
{
    /** Operations of the capture session, for scripting and call counting. */
    enum FakeCaptureHalOperation
    {
        FAKE_CAPTURE_HAL_GET_CAPABILITIES = 0,
        FAKE_CAPTURE_HAL_CREATE_POOL = 1,
        FAKE_CAPTURE_HAL_ACQUIRE_CURRENT_FRAME = 2,
        FAKE_CAPTURE_HAL_RELEASE_FRAME = 3,
        FAKE_CAPTURE_HAL_OPERATION_COUNT = 4
    };

    /** Restores the default behaviour and clears all counters and scripted results. */
    void fakeCaptureHalReset();

    /** Makes createCaptureSession() return null (true) or a session (false, the default). */
    void fakeCaptureHalSetFactoryReturnsNull(bool returnsNull);

    /** Sets the slot limits reported by getCapabilities() and used by createPool(). */
    void fakeCaptureHalSetSlotLimits(uint32_t minimumSlots, uint32_t maximumSlots);

    /** Queues a CaptureStatus value to be returned by the next call of \a operation instead of the default. */
    void fakeCaptureHalPushResult(int operation, uint32_t status);

    /** Makes the next acquireCurrentFrame() wait inside the HAL until fakeCaptureHalUnblockAcquire(). */
    void fakeCaptureHalBlockNextAcquire();

    /** Waits until an acquisition is held by the gate. @retval false on timeout. */
    bool fakeCaptureHalWaitUntilAcquireBlocked(int timeoutMs);

    /** Lets the held acquisition continue. */
    void fakeCaptureHalUnblockAcquire();

    /** Makes the next successfully acquired frame invalid (a plane past the end of its object). */
    void fakeCaptureHalSetNextFrameInvalid(bool invalid);

    /** Number of calls made to \a operation since the last reset. */
    int fakeCaptureHalCallCount(int operation);

    /** Number of sessions created and not yet destroyed. */
    int fakeCaptureHalLiveSessions();

    /** Number of sessions destroyed since the last reset. */
    int fakeCaptureHalDestroyedSessions();

    /** Signatures, for dlsym. */
    typedef void (*FakeCaptureHalResetFunction)();
    typedef void (*FakeCaptureHalSetFactoryReturnsNullFunction)(bool);
    typedef void (*FakeCaptureHalSetSlotLimitsFunction)(uint32_t, uint32_t);
    typedef void (*FakeCaptureHalPushResultFunction)(int, uint32_t);
    typedef int (*FakeCaptureHalCallCountFunction)(int);
    typedef int (*FakeCaptureHalCountFunction)();
    typedef void (*FakeCaptureHalSetNextFrameInvalidFunction)(bool);
    typedef void (*FakeCaptureHalBlockNextAcquireFunction)();
    typedef bool (*FakeCaptureHalWaitUntilAcquireBlockedFunction)(int);
    typedef void (*FakeCaptureHalUnblockAcquireFunction)();
}

#endif // FIREBOLT_RIALTO_CAPTURE_FAKE_CAPTURE_HAL_CONTROL_H_
