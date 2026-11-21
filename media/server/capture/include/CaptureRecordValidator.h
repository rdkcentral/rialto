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

#ifndef FIREBOLT_RIALTO_CAPTURE_CAPTURE_RECORD_VALIDATOR_H_
#define FIREBOLT_RIALTO_CAPTURE_CAPTURE_RECORD_VALIDATOR_H_

#include "ICaptureSession.h"

#include <cstdint>
#include <string>

/**
 * @file CaptureRecordValidator.h
 *
 * Middleware-side checks on the records the capture HAL returns (SoC capture spec 3.1, 3.2, 5.1 and 6.2). The
 * checks are pure: they read the records and report the first rule that fails. Callers decide what to do with a
 * failure (log it, treat the operation as FATAL_ERROR, release a failed frame).
 */

namespace firebolt::rialto::capture
{
/**
 * @brief The outcome of a validation.
 */
struct ValidationResult
{
    bool ok{true};      /**< True if every rule passed. */
    std::string rule{}; /**< Name of the first failing rule, empty when ok. */

    static ValidationResult pass() { return {}; }
    static ValidationResult fail(const std::string &failedRule) { return {false, failedRule}; }
};

/**
 * @brief Checks a successful getCapabilities() record.
 *
 * Rules: the record size is at least \a builtSize; 1 <= minimumSlots <= maximumSlots; maximumContentSize has
 * non-zero width and height.
 *
 * @param[in] capabilities : The record.
 * @param[in] builtSize    : sizeof(CaptureCapabilities) as Rialto was built.
 */
ValidationResult validateCapabilities(const CaptureCapabilities &capabilities,
                                      uint32_t builtSize = sizeof(CaptureCapabilities));

/**
 * @brief Checks a successful createPool() record against the capabilities it was created from.
 *
 * Rules: \a requestedCount is within the capability limits; the pool has exactly \a requestedCount slot layouts;
 * each slotIndex equals its position; backingSize is non-zero and within maximumContentSize; every template plane
 * has object ID zero.
 */
ValidationResult validatePool(const CapturePool &pool, const CaptureCapabilities &capabilities, uint32_t requestedCount);

} // namespace firebolt::rialto::capture

#endif // FIREBOLT_RIALTO_CAPTURE_CAPTURE_RECORD_VALIDATOR_H_
