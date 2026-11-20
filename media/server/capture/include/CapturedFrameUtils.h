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

#ifndef FIREBOLT_RIALTO_CAPTURE_CAPTURED_FRAME_UTILS_H_
#define FIREBOLT_RIALTO_CAPTURE_CAPTURED_FRAME_UTILS_H_

#include "ICaptureSession.h"

/**
 * @file CapturedFrameUtils.h
 *
 * Rialto-private helpers for CapturedFrame records (SoC capture spec section 5.1). They are not part of the
 * vendor-facing HAL header.
 */

namespace firebolt::rialto::capture
{
/**
 * @brief Checks whether a record is empty.
 *
 * A record is empty when its slot index is kInvalidCaptureSlotIndex, its acquisition token is
 * kInvalidAcquisitionToken and every exported descriptor is -1. Any other descriptor value, including a negative
 * value other than -1, makes it non-empty.
 */
bool isEmpty(const CapturedFrame &frame);

/**
 * @brief Resets a released record that holds no descriptors, so it can be passed to acquireCurrentFrame() again.
 *
 * Clears the invalidation flag and all frame metadata. Does nothing and returns false if the record still holds a
 * frame lock (valid slot or token) or any descriptor other than -1.
 *
 * @retval true if the record was reset and is now empty.
 */
bool resetReleased(CapturedFrame &frame);

} // namespace firebolt::rialto::capture

#endif // FIREBOLT_RIALTO_CAPTURE_CAPTURED_FRAME_UTILS_H_
