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
 * capture do not load it. Vulkan capture types are added with Vulkan import.
 *
 * @since V1.1
 */

#include <stdint.h>

#include <memory>
#include <vector>

// Opaque EGL handles, matching <EGL/egl.h> where EGLDisplay and EGLContext are void *.
using EGLDisplay = void *;
using EGLContext = void *;

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
    /** Releases this frame's lock on final shared ownership; compatible imports may remain cached. */
    virtual ~IVideoFrame() = default;
    /** Returns the reusable capture-pool slot index; it is not a globally unique frame ID. */
    virtual uint32_t frameIndex() const = 0;
    /** Returns the number of texture or image resources exposed by this frame. */
    virtual uint32_t numResources() const = 0;
    /** Describes how the frame's resources compose the visible image. */
    virtual GraphicsTextureLayout layout() const = 0;
    /** Returns the selected frame's presentation timestamp in nanoseconds. */
    virtual int64_t presentationTimeStampNs() const = 0;
    /** Returns the padded backing width in pixels. */
    virtual uint32_t textureWidth() const = 0;
    /** Returns the padded backing height in pixels. */
    virtual uint32_t textureHeight() const = 0;
    /** Returns the visible content width in pixels. */
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
     * @return The current retained frame and whether its selection changed.
     */
    virtual GLFrameAcquisition acquireCurrentFrame() = 0;
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
     */
    virtual std::shared_ptr<IGLVideoFrameCapture> createGLVideoFrameCapture(EGLDisplay eglDisplay,
                                                                            EGLContext eglContext) = 0;

protected:
    virtual ~IVideoFrameCaptureFactory() = default;
};

} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_VIDEO_FRAME_CAPTURE_H_
