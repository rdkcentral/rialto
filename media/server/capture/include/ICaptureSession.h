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

#ifndef FIREBOLT_RIALTO_CAPTURE_I_CAPTURE_SESSION_H_
#define FIREBOLT_RIALTO_CAPTURE_I_CAPTURE_SESSION_H_

/**
 * @file ICaptureSession.h
 *
 * The SoC video frame capture HAL interface, as defined by the SoC Video Frame Capture specification
 * (sections 3, 5.1 and 8.1). The vendor capture library implements ICaptureSession and exports the
 * createCaptureSession() factory; Rialto loads the library, creates one session per capture pipeline
 * and calls it.
 *
 * This header is installed for vendor builds. The vendor library and Rialto must be built against the
 * same revision of it, with compatible C++ runtimes. No exception may escape a HAL call; failures are
 * reported through CaptureStatus.
 */

#include <cstdint>
#include <limits>
#include <vector>

namespace firebolt::rialto::capture
{
/**
 * @brief Result of a HAL call.
 */
enum class CaptureStatus : uint32_t
{
    OK,                            /**< Success. */
    NO_NEW_FRAME,                  /**< The current selection was already returned. */
    UNSUPPORTED,                   /**< The operation is not supported. */
    INVALID_ARGUMENT,              /**< An argument, the call order or the record state is invalid. */
    NO_RESOURCES,                  /**< A transient resource failure; the call may be retried. */
    NO_FRAME,                      /**< No valid selection exists. */
    FATAL_ERROR,                   /**< Capture is unavailable for the rest of the session. */
    CAPABILITIES_NOT_AVAILABLE_YET /**< getCapabilities() was called before configured READY. */
};

/** Slot index of a record that holds no frame lock. */
constexpr uint32_t kInvalidCaptureSlotIndex = std::numeric_limits<uint32_t>::max();

/** Acquisition token of a record that holds no frame lock. */
constexpr uint64_t kInvalidAcquisitionToken = 0;

/**
 * @brief A width and height in pixels.
 */
struct Size
{
    uint32_t width;
    uint32_t height;
};

/**
 * @brief A rectangle in pixels.
 */
struct Rectangle
{
    uint32_t x;
    uint32_t y;
    Size size;
};

/**
 * @brief The configured capture format and pool-capacity limits.
 *
 * The limits satisfy 1 <= minimumSlots <= maximumSlots and count retained frames, not DMA-BUF objects.
 */
struct CaptureCapabilities
{
    uint32_t size{sizeof(CaptureCapabilities)}; /**< Size of this record in bytes. */
    uint32_t drmFormat;                         /**< DRM fourcc of captured frames. */
    uint64_t drmModifier;                       /**< DRM format modifier of captured frames. */
    Size maximumContentSize;                    /**< Largest backing size a slot supports. */
    uint32_t minimumSlots;                      /**< Least supported slot count for the configured video. */
    uint32_t maximumSlots;                      /**< Greatest slot count that preserves normal playback. */
};

/**
 * @brief The YCbCr matrix of a captured frame.
 */
enum class CaptureYcbcrMatrix : uint32_t
{
    UNKNOWN = 0,
    BT601 = 1,
    BT709 = 2,
    BT2020 = 3,
};

/**
 * @brief The sample range of a captured frame.
 */
enum class CaptureSampleRange : uint32_t
{
    UNKNOWN = 0,
    FULL = 1,
    NARROW = 2,
};

/**
 * @brief The chroma siting of a captured frame along one axis.
 *
 * COSITED means offset 0 and MIDPOINT means offset 0.5.
 */
enum class CaptureChromaSiting : uint32_t
{
    UNKNOWN = 0,
    COSITED = 1,
    MIDPOINT = 2,
};

/**
 * @brief The effective colour interpretation of a captured frame.
 *
 * UNKNOWN is valid only for an empty record or for fields that do not apply to an RGB format.
 */
struct CaptureColorDescription
{
    CaptureYcbcrMatrix matrix{CaptureYcbcrMatrix::UNKNOWN};
    CaptureSampleRange range{CaptureSampleRange::UNKNOWN};
    CaptureChromaSiting horizontalSiting{CaptureChromaSiting::UNKNOWN};
    CaptureChromaSiting verticalSiting{CaptureChromaSiting::UNKNOWN};
};

/**
 * @brief One DMA-BUF allocation referenced by a frame.
 *
 * Object IDs are opaque and non-zero in acquired frames. Zero denotes an unbound allocation in a pool template.
 */
struct DmaBufObject
{
    uint64_t objectId;  /**< Opaque allocation identity, stable until dmaBufObjectsFlushed. */
    uint64_t sizeBytes; /**< Size available for range validation. */
};

/**
 * @brief One DRM plane of a frame and where it is stored.
 */
struct FramePlane
{
    uint32_t planeIndex;     /**< Plane index in the order defined by the DRM format. */
    uint64_t dmaBufObjectId; /**< The object that holds this plane. */
    uint64_t offsetBytes;    /**< Byte offset of the plane in its object. */
    uint64_t lengthBytes;    /**< Byte length of the plane. */
    uint32_t strideBytes;    /**< Byte stride of the plane. */
};

/**
 * @brief The structural layout of one pool slot.
 */
struct CaptureSlotLayout
{
    uint32_t slotIndex;             /**< Equal to the layout's position in CapturePool::slotLayouts. */
    std::vector<FramePlane> planes; /**< Template planes; object IDs are zero. */
};

/**
 * @brief A file descriptor exported for one DMA-BUF object of an acquired frame.
 *
 * Middleware owns the descriptor and sets fd to -1 once it is closed or consumed by an import.
 */
struct ExportedDmaBuf
{
    uint64_t dmaBufObjectId;
    int fd{-1};
};

/**
 * @brief The admitted pool capacity and its initial, FD-free layout.
 */
struct CapturePool
{
    Size backingSize;                           /**< Configured slot dimension for the admitted capacity. */
    std::vector<CaptureSlotLayout> slotLayouts; /**< One layout per admitted slot. */
};

/**
 * @brief An acquired frame: its description, its frame lock and its exported descriptors.
 *
 * A record is neither copyable nor movable. A valid slotIndex with a matching acquisitionToken means exactly one
 * releaseFrame() is owed. Destroying a record neither closes descriptors nor releases the lock.
 */
struct CapturedFrame
{
    uint32_t slotIndex{kInvalidCaptureSlotIndex};
    uint64_t acquisitionToken{kInvalidAcquisitionToken};
    int64_t presentationTimeNs;
    Rectangle visibleRegion;
    bool dmaBufObjectsFlushed{false};
    CaptureCapabilities capabilities{};
    CaptureColorDescription colorDescription{};
    Size backingSize{};
    std::vector<DmaBufObject> dmaBufObjects;
    std::vector<FramePlane> planes;
    std::vector<ExportedDmaBuf> exportedDmaBufs;

    CapturedFrame() : presentationTimeNs{0}, visibleRegion{} {}
    CapturedFrame(const CapturedFrame &) = delete;
    CapturedFrame &operator=(const CapturedFrame &) = delete;
    CapturedFrame(CapturedFrame &&) = delete;
    CapturedFrame &operator=(CapturedFrame &&) = delete;
    ~CapturedFrame() = default;
};

/**
 * @brief One capture session, created per capture pipeline by createCaptureSession().
 *
 * Middleware serializes calls within a session and deletes it through this interface's virtual destructor.
 */
class ICaptureSession
{
public:
    virtual ~ICaptureSession() = default;

    /**
     * @brief Queries the configured capture format and pool-capacity limits.
     *
     * Returns CAPABILITIES_NOT_AVAILABLE_YET before configured READY. Allocates nothing and exports no descriptors.
     * Every non-OK result leaves \a out unchanged.
     */
    virtual CaptureStatus getCapabilities(CaptureCapabilities &out) const = 0;

    /**
     * @brief Sets the pool capacity once for the session and returns its initial layout.
     *
     * \a slotCount must be within the configured inclusive limits. Every non-OK result leaves \a outPool unchanged.
     */
    virtual CaptureStatus createPool(uint32_t slotCount, CapturePool &outPool) = 0;

    /**
     * @brief Locks the selected frame and returns its description and descriptors.
     *
     * \a outFrame must be empty. Every non-OK result leaves it unchanged and creates no release obligation.
     */
    virtual CaptureStatus acquireCurrentFrame(CapturedFrame &outFrame) = 0;

    /**
     * @brief Ends the frame lock of an acquired record after graphics use has finished.
     *
     * Does not inspect or close descriptors. On OK the record's slot and token are invalidated.
     */
    virtual CaptureStatus releaseFrame(CapturedFrame &frame) = 0;
};

/**
 * @brief Signature of the vendor library's factory.
 */
using CreateCaptureSessionFunction = ICaptureSession *(*)();

} // namespace firebolt::rialto::capture

/**
 * @brief Factory exported by the vendor capture library, lib_capture_hal.so by default.
 *
 * Returns an owned session that Middleware deletes through ICaptureSession's virtual destructor, or nullptr.
 */
extern "C" firebolt::rialto::capture::ICaptureSession *createCaptureSession();

#endif // FIREBOLT_RIALTO_CAPTURE_I_CAPTURE_SESSION_H_
