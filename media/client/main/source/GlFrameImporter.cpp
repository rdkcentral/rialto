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

#include "GlFrameImporter.h"
#include "RialtoClientLogging.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>

namespace firebolt::rialto::client
{
using namespace egl;

namespace
{
// Whether a space-separated extension string contains the whole token.
bool hasExtension(const char *extensions, const char *name)
{
    if (!extensions)
        return false;
    std::istringstream stream{extensions};
    std::string token;
    while (stream >> token)
    {
        if (token == name)
            return true;
    }
    return false;
}
struct PlaneAttributes
{
    EGLint fd;
    EGLint offset;
    EGLint pitch;
    EGLint modifierLo;
    EGLint modifierHi;
};

constexpr PlaneAttributes kPlaneAttributes[]{
    {EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGL_DMA_BUF_PLANE0_PITCH_EXT,
     EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT},
    {EGL_DMA_BUF_PLANE1_FD_EXT, EGL_DMA_BUF_PLANE1_OFFSET_EXT, EGL_DMA_BUF_PLANE1_PITCH_EXT,
     EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT},
    {EGL_DMA_BUF_PLANE2_FD_EXT, EGL_DMA_BUF_PLANE2_OFFSET_EXT, EGL_DMA_BUF_PLANE2_PITCH_EXT,
     EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT},
    {EGL_DMA_BUF_PLANE3_FD_EXT, EGL_DMA_BUF_PLANE3_OFFSET_EXT, EGL_DMA_BUF_PLANE3_PITCH_EXT,
     EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT},
};

// Wire values (proto VideoFrameCapture*): 0 is UNKNOWN in each enum.
std::optional<EGLint> colourSpaceHint(std::uint32_t matrix)
{
    switch (matrix)
    {
    case 1:
        return EGL_ITU_REC601_EXT;
    case 2:
        return EGL_ITU_REC709_EXT;
    case 3:
        return EGL_ITU_REC2020_EXT;
    default:
        return std::nullopt;
    }
}

std::optional<EGLint> rangeHint(std::uint32_t range)
{
    switch (range)
    {
    case 1:
        return EGL_YUV_FULL_RANGE_EXT;
    case 2:
        return EGL_YUV_NARROW_RANGE_EXT;
    default:
        return std::nullopt;
    }
}

std::optional<EGLint> sitingHint(std::uint32_t siting)
{
    switch (siting)
    {
    case 1:
        return EGL_YUV_CHROMA_SITING_0_EXT; // co-sited
    case 2:
        return EGL_YUV_CHROMA_SITING_0_5_EXT; // midpoint
    default:
        return std::nullopt;
    }
}

bool fitsEglInt(std::uint64_t value)
{
    return value <= static_cast<std::uint64_t>(std::numeric_limits<EGLint>::max());
}
} // namespace

std::optional<std::vector<EGLint>> buildEglAttributes(const CaptureFrameDescription &description,
                                                      const std::vector<CaptureObjectFd> &fds)
{
    if (description.planes.empty() || !fitsEglInt(description.backingWidth) || !fitsEglInt(description.backingHeight) ||
        !fitsEglInt(description.drmFormat))
    {
        return std::nullopt;
    }
    std::vector<EGLint> attributes{EGL_WIDTH,
                                   static_cast<EGLint>(description.backingWidth),
                                   EGL_HEIGHT,
                                   static_cast<EGLint>(description.backingHeight),
                                   EGL_LINUX_DRM_FOURCC_EXT,
                                   static_cast<EGLint>(description.drmFormat)};
    const EGLint kModifierLo{static_cast<EGLint>(description.drmModifier & 0xffffffffU)};
    const EGLint kModifierHi{static_cast<EGLint>(description.drmModifier >> 32)};

    bool planeSeen[std::size(kPlaneAttributes)]{};
    for (const CapturePlane &kPlane : description.planes)
    {
        if (kPlane.planeIndex >= std::size(kPlaneAttributes) || planeSeen[kPlane.planeIndex] ||
            !fitsEglInt(kPlane.offsetBytes) || !fitsEglInt(kPlane.strideBytes))
        {
            return std::nullopt;
        }
        planeSeen[kPlane.planeIndex] = true;
        auto fdIt = std::find_if(fds.begin(), fds.end(),
                                 [&kPlane](const CaptureObjectFd &kFd) { return kFd.objectId == kPlane.objectId; });
        if (fdIt == fds.end() || fdIt->fd < 0)
        {
            return std::nullopt;
        }
        const PlaneAttributes &kNames{kPlaneAttributes[kPlane.planeIndex]};
        attributes.insert(attributes.end(), {kNames.fd, fdIt->fd, kNames.offset, static_cast<EGLint>(kPlane.offsetBytes),
                                             kNames.pitch, static_cast<EGLint>(kPlane.strideBytes), kNames.modifierLo,
                                             kModifierLo, kNames.modifierHi, kModifierHi});
    }

    const std::pair<EGLint, std::optional<EGLint>> kHints[]{
        {EGL_YUV_COLOR_SPACE_HINT_EXT, colourSpaceHint(description.colour.matrix)},
        {EGL_SAMPLE_RANGE_HINT_EXT, rangeHint(description.colour.range)},
        {EGL_YUV_CHROMA_HORIZONTAL_SITING_HINT_EXT, sitingHint(description.colour.horizontalSiting)},
        {EGL_YUV_CHROMA_VERTICAL_SITING_HINT_EXT, sitingHint(description.colour.verticalSiting)},
    };
    for (const auto &[kName, kValue] : kHints)
    {
        if (kValue)
            attributes.insert(attributes.end(), {kName, *kValue});
    }
    attributes.push_back(EGL_NONE);
    return attributes;
}

GlFrameImporter::GlFrameImporter(const std::shared_ptr<IEglGl> &gl, EGLDisplay display, EGLContext context)
    : m_gl{gl}, m_display{display}, m_context{context}
{
}

bool GlFrameImporter::hasRequiredExtensions()
{
    if (!m_extensionsPresent)
    {
        const char *kEglExtensions = m_gl->eglQueryString(m_display, EGL_EXTENSIONS);
        const char *kGlExtensions = reinterpret_cast<const char *>(m_gl->glGetString(GL_EXTENSIONS));
        bool present{true};
        for (const char *kName : {"EGL_EXT_image_dma_buf_import", "EGL_EXT_image_dma_buf_import_modifiers"})
        {
            if (!hasExtension(kEglExtensions, kName))
            {
                RIALTO_CLIENT_LOG_ERROR("GL video frame capture needs %s, which the display lacks", kName);
                present = false;
            }
        }
        if (!hasExtension(kGlExtensions, "GL_OES_EGL_image_external"))
        {
            RIALTO_CLIENT_LOG_ERROR("GL video frame capture needs GL_OES_EGL_image_external, which the context lacks");
            present = false;
        }
        m_extensionsPresent = present; // checked once: later imports fail fast and silently
    }
    return *m_extensionsPresent;
}

bool GlFrameImporter::isSupported(std::uint32_t drmFormat, std::uint64_t drmModifier)
{
    if (!m_gl || !hasRequiredExtensions())
    {
        return false;
    }
    const auto kPair = std::make_pair(drmFormat, drmModifier);
    auto it = m_supportedPairs.find(kPair);
    if (it != m_supportedPairs.end())
    {
        return it->second;
    }

    bool supported{false};
    EGLint count{0};
    if (m_gl->eglQueryDmaBufModifiersEXT(m_display, static_cast<EGLint>(drmFormat), 0, nullptr, nullptr, &count) &&
        count > 0)
    {
        std::vector<EGLuint64KHR> modifiers(static_cast<std::size_t>(count));
        std::vector<EGLBoolean> externalOnly(static_cast<std::size_t>(count));
        if (m_gl->eglQueryDmaBufModifiersEXT(m_display, static_cast<EGLint>(drmFormat), count, modifiers.data(),
                                             externalOnly.data(), &count))
        {
            modifiers.resize(static_cast<std::size_t>(std::max(count, 0)));
            supported = std::find(modifiers.begin(), modifiers.end(), drmModifier) != modifiers.end();
        }
    }
    if (!supported)
    {
        RIALTO_CLIENT_LOG_ERROR("The display cannot import format 0x%08x with modifier 0x%016llx", drmFormat,
                                static_cast<unsigned long long>(drmModifier));
    }
    m_supportedPairs.emplace(kPair, supported);
    return supported;
}

std::shared_ptr<IImportedImage> GlFrameImporter::importFrame(const CaptureFrameDescription &description,
                                                             const std::vector<CaptureObjectFd> &fds, bool &fdsConsumed)
{
    fdsConsumed = false; // EGL borrows descriptors; the caller closes them (design D5)
    (void)fds;
    if (!isSupported(description.drmFormat, description.drmModifier))
    {
        return nullptr;
    }
    return nullptr; // the import itself follows in task 2.2
}

void GlFrameImporter::retire(const std::shared_ptr<IImportedImage> &image)
{
    (void)image;
}

void GlFrameImporter::drain() {}
} // namespace firebolt::rialto::client
