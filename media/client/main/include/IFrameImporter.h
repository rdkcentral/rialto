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

#ifndef FIREBOLT_RIALTO_CLIENT_I_FRAME_IMPORTER_H_
#define FIREBOLT_RIALTO_CLIENT_I_FRAME_IMPORTER_H_

#include "IVideoFrameCaptureIpc.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace firebolt::rialto::client
{
/**
 * @brief A graphics import of one frame layout: the textures or images an application samples.
 */
class IImportedImage
{
public:
    IImportedImage() = default;
    virtual ~IImportedImage() = default;

    IImportedImage(const IImportedImage &) = delete;
    IImportedImage &operator=(const IImportedImage &) = delete;
    IImportedImage(IImportedImage &&) = delete;
    IImportedImage &operator=(IImportedImage &&) = delete;

    /** GL texture target of the import (GL importers only). */
    virtual std::uint32_t textureTarget() const = 0;

    /** GL texture IDs of the import (GL importers only). */
    virtual const std::vector<std::uint32_t> &textureIds() const = 0;
};

/**
 * @brief Graphics-API-specific import of captured frames, used by CaptureCore.
 *
 * importFrame() runs inside acquireCurrentFrame(), where the application's graphics context is current. retire()
 * may be called on any thread and must not need a context; drain() runs at the start of each acquisition and
 * destroys what was retired.
 */
class IFrameImporter
{
public:
    IFrameImporter() = default;
    virtual ~IFrameImporter() = default;

    IFrameImporter(const IFrameImporter &) = delete;
    IFrameImporter &operator=(const IFrameImporter &) = delete;
    IFrameImporter(IFrameImporter &&) = delete;
    IFrameImporter &operator=(IFrameImporter &&) = delete;

    /**
     * @brief Imports a frame layout.
     *
     * @param[in]  description  : The frame description.
     * @param[in]  fds          : The received descriptors, one per distinct object.
     * @param[out] fdsConsumed  : True if the import took ownership of the descriptors (for example Vulkan memory
     *                            import); false if it only borrowed them (EGL), so the caller closes them.
     *
     * @retval the import, or null on failure.
     */
    virtual std::shared_ptr<IImportedImage> importFrame(const CaptureFrameDescription &description,
                                                        const std::vector<CaptureObjectFd> &fds, bool &fdsConsumed) = 0;

    /** Queues an import for destruction. Must not need a graphics context. */
    virtual void retire(const std::shared_ptr<IImportedImage> &image) = 0;

    /** Destroys queued imports. Called with the application's graphics context current. */
    virtual void drain() = 0;

    /**
     * @brief Offers to send a frame's slot release later (R13 design D5).
     *
     * Called on the thread that drops the frame's last reference. An importer that must first give the image back to
     * the producer (Vulkan: a release barrier and its fence) keeps sendRelease and calls it exactly once, later.
     *
     * @param[in] image       : The frame's import.
     * @param[in] sendRelease : Sends the slot release. Must be called exactly once if this returns true.
     *
     * @retval true if the importer took sendRelease; false to have the caller send the release now (GL).
     */
    virtual bool deferRelease(const std::shared_ptr<IImportedImage> &image, std::function<void()> sendRelease)
    {
        (void)image;
        (void)sendRelease;
        return false;
    }
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_FRAME_IMPORTER_H_
