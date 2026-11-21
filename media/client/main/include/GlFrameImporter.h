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

#ifndef FIREBOLT_RIALTO_CLIENT_GL_FRAME_IMPORTER_H_
#define FIREBOLT_RIALTO_CLIENT_GL_FRAME_IMPORTER_H_

#include "IEglGl.h"
#include "IFrameImporter.h"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace firebolt::rialto::client
{
/**
 * @brief Builds the EGL_LINUX_DMA_BUF_EXT attribute list for a frame (SoC spec 6.3, design D3).
 *
 * The image is the backing size. Each plane uses its object's descriptor (repeated when planes share an object),
 * its own offset and pitch, and the frame's modifier as LO/HI. Colour hints come from the colour description and
 * are left out where it is UNKNOWN (RGB formats). The list ends with EGL_NONE.
 *
 * @retval nothing if a plane has no descriptor, a plane index is repeated or above 3, or a value does not fit.
 */
std::optional<std::vector<egl::EGLint>> buildEglAttributes(const CaptureFrameDescription &description,
                                                           const std::vector<CaptureObjectFd> &fds);

/**
 * @brief One frame layout imported as an EGL image bound to one GL_TEXTURE_EXTERNAL_OES texture.
 *
 * Destroying it makes no EGL or GL call: the importer deletes the texture and image (design D4).
 */
class GlImportedImage : public IImportedImage
{
public:
    GlImportedImage(egl::EGLImage image, egl::GLuint texture) : m_image{image}, m_textureIds{texture} {}

    std::uint32_t textureTarget() const override { return egl::GL_TEXTURE_EXTERNAL_OES; }
    const std::vector<std::uint32_t> &textureIds() const override { return m_textureIds; }

    egl::EGLImage eglImage() const { return m_image; }
    egl::GLuint texture() const { return m_textureIds.front(); }

private:
    const egl::EGLImage m_image;
    const std::vector<std::uint32_t> m_textureIds;
};

/**
 * @brief Imports captured frames as EGL images bound to GL_TEXTURE_EXTERNAL_OES textures (SoC spec 6.3, 6.4).
 */
class GlFrameImporter : public IFrameImporter
{
public:
    /**
     * @param[in] gl      : The EGL and GL entry points.
     * @param[in] display : The application's EGL display.
     * @param[in] context : The application's EGL context; the capture's share group.
     */
    GlFrameImporter(const std::shared_ptr<IEglGl> &gl, egl::EGLDisplay display, egl::EGLContext context);
    ~GlFrameImporter() override = default;

    std::shared_ptr<IImportedImage> importFrame(const CaptureFrameDescription &description,
                                                const std::vector<CaptureObjectFd> &fds, bool &fdsConsumed) override;
    void retire(const std::shared_ptr<IImportedImage> &image) override;
    void drain() override;

    /**
     * @brief Whether frames of this format and modifier can be imported on the configured display (design D2).
     *
     * The extensions are checked once per capture and each format/modifier pair once; a failure is logged once.
     * Must be called with the application's context current.
     */
    bool isSupported(std::uint32_t drmFormat, std::uint64_t drmModifier);

private:
    bool hasRequiredExtensions();

    /** Deletes the texture, then the image (SoC spec 6.4 step 5). Needs a context of the share group current. */
    void destroyImport(egl::GLuint texture, egl::EGLImage image);

    std::shared_ptr<IEglGl> m_gl;
    egl::EGLDisplay m_display;
    egl::EGLContext m_context;
    std::optional<bool> m_extensionsPresent;
    std::map<std::pair<std::uint32_t, std::uint64_t>, bool> m_supportedPairs;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_GL_FRAME_IMPORTER_H_
