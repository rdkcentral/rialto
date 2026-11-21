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

#ifndef FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_H_
#define FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_H_

#include "FrameLease.h"
#include "IFrameImporter.h"
#include "IVideoFrameCapture.h"
#include "IVideoFrameCaptureIpc.h"
#include "ImportCache.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace firebolt::rialto::client
{
/**
 * @brief A frame returned to the application. Holding a reference holds the frame's capture slot (the lease):
 * when the last reference goes, the slot is released.
 */
class VideoFrame : public IGLVideoFrame
{
public:
    VideoFrame(const CaptureFrameDescription &description, const std::shared_ptr<ImportCacheEntry> &import,
               std::unique_ptr<FrameLease> lease = nullptr);
    ~VideoFrame() override = default;

    std::uint32_t frameIndex() const override { return m_description.slotIndex; }
    std::uint32_t numResources() const override { return 1; }
    GraphicsTextureLayout layout() const override { return GraphicsTextureLayout::RGBA; }
    std::int64_t presentationTimeStampNs() const override { return m_description.presentationTimeNs; }
    std::uint32_t textureWidth() const override { return m_description.backingWidth; }
    std::uint32_t textureHeight() const override { return m_description.backingHeight; }
    std::uint32_t visibleWidth() const override { return m_description.visibleWidth; }
    std::uint32_t visibleHeight() const override { return m_description.visibleHeight; }
    std::uint32_t textureTarget() const override;
    const std::vector<std::uint32_t> &textureIds() const override;

    /** The description the server sent for this frame. */
    const CaptureFrameDescription &description() const { return m_description; }

private:
    const CaptureFrameDescription m_description;
    std::shared_ptr<ImportCacheEntry> m_import; /**< Keeps the import alive while the frame exists. */
    std::unique_ptr<FrameLease> m_lease;        /**< Released when the last reference to the frame goes. */
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_H_
