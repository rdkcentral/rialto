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

#ifndef FIREBOLT_RIALTO_CLIENT_EGL_GL_TYPES_H_
#define FIREBOLT_RIALTO_CLIENT_EGL_GL_TYPES_H_

/**
 * @file EglGlTypes.h
 *
 * The EGL and GL types and constants Rialto uses, with their Khronos values, so the client builds without EGL or
 * GLES development headers and has no link-time dependency on either library.
 */

#include <cstdint>

namespace firebolt::rialto::client::egl
{
using EGLBoolean = unsigned int;
using EGLint = std::int32_t;
using EGLenum = unsigned int;
using EGLDisplay = void *;
using EGLContext = void *;
using EGLSurface = void *;
using EGLConfig = void *;
using EGLImage = void *;
using EGLClientBuffer = void *;
using EGLuint64KHR = std::uint64_t;
using GLuint = unsigned int;
using GLenum = unsigned int;
using GLint = int;
using GLsizei = int;
using GLubyte = unsigned char;
using GLeglImageOES = void *;

constexpr EGLBoolean EGL_FALSE{0};
constexpr EGLBoolean EGL_TRUE{1};
constexpr EGLint EGL_SUCCESS{0x3000};
constexpr EGLint EGL_NONE{0x3038};
constexpr EGLint EGL_EXTENSIONS{0x3055};
constexpr EGLint EGL_CONFIG_ID{0x3028};
constexpr EGLint EGL_WIDTH{0x3057};
constexpr EGLint EGL_HEIGHT{0x3056};
constexpr EGLint EGL_CONTEXT_CLIENT_VERSION{0x3098};
constexpr EGLenum EGL_OPENGL_ES_API{0x30A0};
constexpr EGLenum EGL_LINUX_DMA_BUF_EXT{0x3270};
constexpr EGLint EGL_LINUX_DRM_FOURCC_EXT{0x3271};
constexpr EGLint EGL_DMA_BUF_PLANE0_FD_EXT{0x3272};
constexpr EGLint EGL_DMA_BUF_PLANE0_OFFSET_EXT{0x3273};
constexpr EGLint EGL_DMA_BUF_PLANE0_PITCH_EXT{0x3274};
constexpr EGLint EGL_DMA_BUF_PLANE1_FD_EXT{0x3275};
constexpr EGLint EGL_DMA_BUF_PLANE1_OFFSET_EXT{0x3276};
constexpr EGLint EGL_DMA_BUF_PLANE1_PITCH_EXT{0x3277};
constexpr EGLint EGL_DMA_BUF_PLANE2_FD_EXT{0x3278};
constexpr EGLint EGL_DMA_BUF_PLANE2_OFFSET_EXT{0x3279};
constexpr EGLint EGL_DMA_BUF_PLANE2_PITCH_EXT{0x327A};
constexpr EGLint EGL_YUV_COLOR_SPACE_HINT_EXT{0x327B};
constexpr EGLint EGL_SAMPLE_RANGE_HINT_EXT{0x327C};
constexpr EGLint EGL_YUV_CHROMA_HORIZONTAL_SITING_HINT_EXT{0x327D};
constexpr EGLint EGL_YUV_CHROMA_VERTICAL_SITING_HINT_EXT{0x327E};
constexpr EGLint EGL_ITU_REC601_EXT{0x327F};
constexpr EGLint EGL_ITU_REC709_EXT{0x3280};
constexpr EGLint EGL_ITU_REC2020_EXT{0x3281};
constexpr EGLint EGL_YUV_FULL_RANGE_EXT{0x3282};
constexpr EGLint EGL_YUV_NARROW_RANGE_EXT{0x3283};
constexpr EGLint EGL_YUV_CHROMA_SITING_0_EXT{0x3284};
constexpr EGLint EGL_YUV_CHROMA_SITING_0_5_EXT{0x3285};
constexpr EGLint EGL_DMA_BUF_PLANE3_FD_EXT{0x3440};
constexpr EGLint EGL_DMA_BUF_PLANE3_OFFSET_EXT{0x3441};
constexpr EGLint EGL_DMA_BUF_PLANE3_PITCH_EXT{0x3442};
constexpr EGLint EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT{0x3443};
constexpr EGLint EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT{0x3444};
constexpr EGLint EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT{0x3445};
constexpr EGLint EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT{0x3446};
constexpr EGLint EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT{0x3447};
constexpr EGLint EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT{0x3448};
constexpr EGLint EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT{0x3449};
constexpr EGLint EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT{0x344A};

const EGLDisplay EGL_NO_DISPLAY{nullptr};
const EGLContext EGL_NO_CONTEXT{nullptr};
const EGLSurface EGL_NO_SURFACE{nullptr};
const EGLImage EGL_NO_IMAGE{nullptr};

constexpr GLenum GL_TEXTURE_EXTERNAL_OES{0x8D65};
constexpr GLenum GL_TEXTURE_MIN_FILTER{0x2801};
constexpr GLenum GL_TEXTURE_MAG_FILTER{0x2800};
constexpr GLenum GL_TEXTURE_WRAP_S{0x2802};
constexpr GLenum GL_TEXTURE_WRAP_T{0x2803};
constexpr GLint GL_LINEAR{0x2601};
constexpr GLint GL_CLAMP_TO_EDGE{0x812F};
constexpr GLenum GL_EXTENSIONS{0x1F03};
constexpr GLenum GL_NO_ERROR{0};
} // namespace firebolt::rialto::client::egl

#endif // FIREBOLT_RIALTO_CLIENT_EGL_GL_TYPES_H_
