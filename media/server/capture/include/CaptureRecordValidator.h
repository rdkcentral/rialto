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
#include <vector>

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

/**
 * @brief Checks a successful acquireCurrentFrame() record.
 *
 * Identity and descriptor rules: the slot index is below \a admittedSlots; the acquisition token is non-zero; the
 * frame has planes; every object ID is non-zero and listed once; every plane references a listed object and every
 * listed object is referenced; exportedDmaBufs has exactly one valid descriptor for each referenced object, with no
 * duplicates and no unreferenced objects.
 *
 * Range rules, checked without arithmetic overflow: every plane satisfies offsetBytes <= sizeBytes and
 * lengthBytes <= sizeBytes - offsetBytes for its object; the visible region fits inside backingSize from its
 * origin.
 *
 * Colour rule: a frame whose DRM format is not a known RGB format must have a known matrix, range and both chroma
 * sitings (SoC capture spec 6.2). Unknown formats are treated as YUV.
 *
 * If \a lockedRanges is given, a plane must not overlap a plane of a different locked slot in the same object.
 */
class LockedRanges;

ValidationResult validateFrame(const CapturedFrame &frame, uint32_t admittedSlots,
                               const LockedRanges *lockedRanges = nullptr);

/**
 * @brief The byte ranges of the frames a session currently holds locked, per DMA-BUF object.
 *
 * Used to reject a new frame whose plane overlaps a plane of a different locked slot in the same object (SoC
 * capture spec 5.1). Object IDs may be rebound after dmaBufObjectsFlushed, so a flushed frame starts a new
 * generation: frames are compared only with locked frames of the same generation. Not thread safe; the session
 * serializes its use.
 */
class LockedRanges
{
public:
    /** Records the planes of a frame that was delivered and is now locked. */
    void add(const CapturedFrame &frame);

    /** Forgets the planes of a released slot. */
    void remove(uint32_t slotIndex);

    /** True if a plane of \a frame overlaps a plane of a different locked slot in the same object. */
    bool overlaps(const CapturedFrame &frame) const;

private:
    struct Range
    {
        uint64_t generation;
        uint32_t slotIndex;
        uint64_t objectId;
        uint64_t offset;
        uint64_t length;
    };

    uint64_t m_generation{0};
    std::vector<Range> m_ranges;
};

} // namespace firebolt::rialto::capture

#endif // FIREBOLT_RIALTO_CAPTURE_CAPTURE_RECORD_VALIDATOR_H_
