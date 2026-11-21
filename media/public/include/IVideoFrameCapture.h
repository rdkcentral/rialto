/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#ifndef FIREBOLT_RIALTO_I_VIDEO_FRAME_CAPTURE_H_
#define FIREBOLT_RIALTO_I_VIDEO_FRAME_CAPTURE_H_

/**
 * @file IVideoFrameCapture.h
 *
 * The proposed Rialto V1.1 definition of the Video Frame Capture API.
 *
 * A Video Frame Capture receives the decoded video of one media pipeline as retained
 * GL textures instead of presenting it on the video plane. It is created by
 * IVideoFrameCaptureFactory and bound with IMediaPipeline::attachVideoFrameCapture().
 *
 * EGL headers are not required: EGLDisplay and EGLContext are declared here as opaque handles.
 * libEGL is loaded at run time when a GL capture is created; applications that do not create a
 * capture do not load it.
 *
 * Vulkan headers are not required either: the Vulkan handles used here are declared exactly as
 * <vulkan/vulkan_core.h> declares them, so the two headers can be included in either order.
 * VkFormat and VkImageLayout values travel as int32_t. libvulkan is loaded at run time when a
 * Vulkan capture is created.
 *
 * @since V1.1
 */

#include <stdint.h>

#include <memory>
#include <vector>

/** Opaque EGL display handle, matching <EGL/egl.h> where EGLDisplay is void *. */
using EGLDisplay = void *;
/** Opaque EGL context handle, matching <EGL/egl.h> where EGLContext is void *. */
using EGLContext = void *;

// Vulkan handles, declared as <vulkan/vulkan_core.h> declares them (an identical redeclaration is valid).
/** Vulkan VkPhysicalDevice handle, as in <vulkan/vulkan_core.h>. */
typedef struct VkPhysicalDevice_T *VkPhysicalDevice;
/** Vulkan VkDevice handle, as in <vulkan/vulkan_core.h>. */
typedef struct VkDevice_T *VkDevice;
/** Vulkan VkQueue handle, as in <vulkan/vulkan_core.h>. */
typedef struct VkQueue_T *VkQueue;
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__)) || defined(_M_X64) ||         \
    defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) || defined(__powerpc64__) ||                           \
    (defined(__riscv) && __riscv_xlen == 64)
/** Vulkan VkImage handle, as in <vulkan/vulkan_core.h>. */
typedef struct VkImage_T *VkImage;
/** Vulkan VkImageView handle, as in <vulkan/vulkan_core.h>. */
typedef struct VkImageView_T *VkImageView;
/** Vulkan VkSampler handle, as in <vulkan/vulkan_core.h>. */
typedef struct VkSampler_T *VkSampler;
/** Vulkan VkSamplerYcbcrConversion handle, as in <vulkan/vulkan_core.h>. */
typedef struct VkSamplerYcbcrConversion_T *VkSamplerYcbcrConversion;
#else
/** Vulkan VkImage handle, as in <vulkan/vulkan_core.h>. */
typedef uint64_t VkImage;
/** Vulkan VkImageView handle, as in <vulkan/vulkan_core.h>. */
typedef uint64_t VkImageView;
/** Vulkan VkSampler handle, as in <vulkan/vulkan_core.h>. */
typedef uint64_t VkSampler;
/** Vulkan VkSamplerYcbcrConversion handle, as in <vulkan/vulkan_core.h>. */
typedef uint64_t VkSamplerYcbcrConversion;
#endif

namespace firebolt::rialto
{
/**
 * @brief The graphics API a capture is bound to.
 */
enum class GraphicsAPIType
{
    GL,    /**< EGL/OpenGL ES textures */
    VULKAN /**< Vulkan images */
};

/**
 * @brief How the resources of one frame compose the picture.
 *
 * This is not a GL internal format, VkFormat or DMA-BUF format.
 */
enum class GraphicsTextureLayout
{
    RGBA,               /**< One resource; colour from RGB and alpha from A. */
    RGBA_SEPARATE_ALPHA /**< Two resources; colour from resource 0 RGB and alpha from resource 1 R. */
};

/**
 * @brief Represents a retained decoded video frame and its common metadata.
 *
 * Holding a shared frame handle retains the selected frame and its graphics resources,
 * preventing the SoC from reusing the associated capture slot. The consumer must keep
 * the handle until all graphics work that can access the frame has completed. When the
 * final handle is released, Rialto releases the SoC slot lock. Compatible graphics
 * imports may remain cached for later acquisitions, but must not be sampled after
 * release. Resolution changes retire unused imports without invalidating retained
 * frames. The API does not wait for queued graphics work on the consumer's behalf.
 */
/// @since V1.1
class IVideoFrame
{
public:
    /**
     * Releases this frame's lock on final shared ownership; compatible imports may remain cached.
     *
     * The frame reference is the lease on its capture slot: while any reference exists the slot is not reused, and
     * dropping the last one releases it. The release may happen on any thread and makes no graphics calls.
     */
    virtual ~IVideoFrame() = default;
    /** Returns the reusable capture-pool slot index; it is not a globally unique frame ID. */
    virtual uint32_t frameIndex() const = 0;
    /** Returns the number of texture or image resources exposed by this frame. */
    virtual uint32_t numResources() const = 0;
    /** Describes how the frame's resources compose the visible image. In V1.1 this is always RGBA with one resource. */
    virtual GraphicsTextureLayout layout() const = 0;
    /** Returns the selected frame's presentation timestamp in nanoseconds. */
    virtual int64_t presentationTimeStampNs() const = 0;
    /** Returns the padded backing width in pixels. */
    virtual uint32_t textureWidth() const = 0;
    /** Returns the padded backing height in pixels. */
    virtual uint32_t textureHeight() const = 0;
    /**
     * Returns the visible content width in pixels. The visible region starts at the texture origin (0, 0) and covers
     * visibleWidth() x visibleHeight() of the textureWidth() x textureHeight() backing.
     */
    virtual uint32_t visibleWidth() const = 0;
    /** Returns the visible content height in pixels. */
    virtual uint32_t visibleHeight() const = 0;
};

/**
 * @brief Represents a retained decoded video frame exposed as GL textures.
 */
/// @since V1.1
class IGLVideoFrame : public IVideoFrame
{
public:
    /** Returns the GL target used to bind each texture ID. */
    virtual uint32_t textureTarget() const = 0;
    /** Returns this frame's texture IDs. The reference is valid while the frame is retained. */
    virtual const std::vector<uint32_t> &textureIds() const = 0;
};

/**
 * @brief One sampleable image of a Vulkan frame.
 *
 * Sample the image through imageView with exactly this sampler: for YCbCr formats the sampler carries Rialto's
 * VkSamplerYcbcrConversion (ycbcrConversion), and the view was created with it. All four handles belong to Rialto
 * and stay valid while the frame is retained; do not destroy them. ycbcrConversion is VK_NULL_HANDLE for RGB
 * formats.
 */
/// @since V1.1
struct VulkanImageResource
{
    VkImage image;                            /**< The imported image. */
    VkImageView imageView;                    /**< A view created with the conversion, for sampling. */
    VkSampler sampler;                        /**< The sampler to use with imageView. */
    VkSamplerYcbcrConversion ycbcrConversion; /**< The YCbCr conversion, or VK_NULL_HANDLE for RGB. */
};

/**
 * @brief Represents a retained decoded video frame exposed as Vulkan images.
 *
 * The images are in imageLayout(), the requiredImageLayout given at creation, and owned by the queue family given
 * at creation: Rialto recorded the queue-family-ownership transfer from VK_QUEUE_FAMILY_FOREIGN_EXT and the layout
 * transition on the capture's queue during acquireCurrentFrame(). Submit work using a frame to that queue after
 * acquireCurrentFrame() returned, and drop the last reference only after that work has completed.
 */
/// @since V1.1
class IVulkanVideoFrame : public IVideoFrame
{
public:
    /** Returns the image format, a VkFormat value. */
    virtual int32_t imageFormat() const = 0;
    /** Returns the layout the images are in, a VkImageLayout value: requiredImageLayout from creation. */
    virtual int32_t imageLayout() const = 0;
    /** Returns this frame's images. In V1.1 there is one, and layout() is RGBA. */
    virtual const std::vector<VulkanImageResource> &images() const = 0;
};

/**
 * @brief Contains the current retained Vulkan frame and whether the scheduler selection changed.
 */
/// @since V1.1
struct VulkanFrameAcquisition
{
    /** True only when this call returns a newly selected scheduler frame. */
    bool changed;
    /** Retained frame for the current selection, as for GLFrameAcquisition. */
    std::shared_ptr<IVulkanVideoFrame> frame;
};

/**
 * @brief Contains the current retained GL frame and whether the scheduler selection changed.
 */
/// @since V1.1
struct GLFrameAcquisition
{
    /** True only when this call returns a newly selected scheduler frame. */
    bool changed;
    /**
     * Retained frame for the current selection. NO_NEW_FRAME returns the cached
     * frame with changed=false; NO_FRAME returns nullptr with changed=false.
     */
    std::shared_ptr<IGLVideoFrame> frame;
};

/**
 * @class IVideoFrameCapture
 * @brief Base interface for a Video Frame Capture configured for one graphics API.
 */
/// @since V1.1
class IVideoFrameCapture
{
public:
    /** Releases the capture object and its current-frame reference. */
    virtual ~IVideoFrameCapture() = default;
    /** Returns the immutable GL or Vulkan API selected at factory creation. */
    virtual GraphicsAPIType apiType() const = 0;
};

/**
 * @brief Provides retained-frame acquisition for a GL capture.
 */
/// @since V1.1
class IGLVideoFrameCapture : public IVideoFrameCapture
{
public:
    /**
     * @brief Acquires the current STC-selected GL frame.
     *
     * A newly selected frame returns changed=true. NO_NEW_FRAME returns the cached
     * frame with changed=false; NO_FRAME returns a null frame with changed=false.
     * Keep the frame handle until GL work using it has completed, then release the
     * final reference to release the slot. Compatible imports may remain cached,
     * but released pixels must not be accessed until the backing is reacquired.
     *
     * Call this with a context of the capture's share group current: Rialto creates
     * textures here and deletes the ones it no longer needs here. Releasing a frame
     * and destroying the capture need no current context and make no GL calls on the
     * releasing thread; Rialto deletes what remains when the capture and its last
     * frame are gone, on a temporary shared context if the creation context is not
     * current then.
     * @return The current retained frame and whether its selection changed.
     */
    virtual GLFrameAcquisition acquireCurrentFrame() = 0;
};

/**
 * @brief Provides retained-frame acquisition for a Vulkan capture.
 */
/// @since V1.1
class IVulkanVideoFrameCapture : public IVideoFrameCapture
{
public:
    /**
     * @brief Acquires the current STC-selected Vulkan frame, as IGLVideoFrameCapture does for GL.
     *
     * Records and submits Rialto's barriers on the capture's queue, so call it on the thread that owns that queue.
     * Releasing a frame needs no queue: its slot is given back after Rialto's release barrier has executed, during a
     * later acquisition or at teardown.
     * @return The current retained frame and whether its selection changed.
     */
    virtual VulkanFrameAcquisition acquireCurrentFrame() = 0;
};

/**
 * @brief Creates Video Frame Capture objects bound to consumer graphics resources.
 */
/// @since V1.1
class IVideoFrameCaptureFactory
{
public:
    /** Returns a factory, or nullptr when Video Frame Capture is unavailable. */
    static std::shared_ptr<IVideoFrameCaptureFactory> createFactory();

    /**
     * @brief Creates a GL capture object bound to an EGL display and context.
     *
     * The EGL context/share group must outlive the capture object and its retained frames.
     * @param[in] eglDisplay EGL display used to import and manage frame resources.
     * @param[in] eglContext EGL context/share group used for GL resources.
     * @return A GL capture object, or nullptr if unsupported or creation fails.
     *
     * The display and the context's share group must stay valid until the capture and
     * every frame acquired from it have been released; destroying them first is
     * undefined behaviour. Rialto loads libEGL only when a GL capture is created.
     */
    virtual std::shared_ptr<IGLVideoFrameCapture> createGLVideoFrameCapture(EGLDisplay eglDisplay,
                                                                            EGLContext eglContext) = 0;

    /**
     * @brief Creates a Vulkan capture object for a device and one of its queues.
     *
     * The device must be Vulkan 1.1 or later with the samplerYcbcrConversion feature and the
     * VK_KHR_external_memory_fd, VK_EXT_external_memory_dma_buf, VK_EXT_image_drm_format_modifier and
     * VK_EXT_queue_family_foreign extensions enabled. Rialto records its ownership-transfer and layout barriers on
     * \a queue, so the application must not use \a queue from another thread during acquireCurrentFrame().
     * The device and queue must outlive the capture and every frame acquired from it.
     *
     * @param[in] physicalDevice      The physical device of \a device.
     * @param[in] device              The logical device the frames are imported into.
     * @param[in] queue               The queue the application samples frames on.
     * @param[in] queueFamilyIndex    The family of \a queue (Vulkan cannot report it).
     * @param[in] requiredImageLayout The VkImageLayout frames are delivered in, for example
     *                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
     * @return A Vulkan capture object, or nullptr if Vulkan cannot be loaded, the device lacks a requirement, or
     *         creation fails.
     */
    virtual std::shared_ptr<IVulkanVideoFrameCapture> createVulkanVideoFrameCapture(VkPhysicalDevice physicalDevice,
                                                                                    VkDevice device, VkQueue queue,
                                                                                    uint32_t queueFamilyIndex,
                                                                                    int32_t requiredImageLayout) = 0;

protected:
    virtual ~IVideoFrameCaptureFactory() = default;
};

} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_VIDEO_FRAME_CAPTURE_H_
