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

#ifndef FIREBOLT_RIALTO_CLIENT_I_EGL_GL_H_
#define FIREBOLT_RIALTO_CLIENT_I_EGL_GL_H_

#include "EglGlTypes.h"

namespace firebolt::rialto::client
{
/**
 * @brief The EGL and GL entry points used by GL video frame capture (design D1).
 *
 * The implementation resolves them from libEGL at run time; tests replace the whole table with a mock.
 */
class IEglGl
{
public:
    virtual ~IEglGl() = default;

    // EGL
    virtual egl::EGLint eglGetError() = 0;
    virtual const char *eglQueryString(egl::EGLDisplay display, egl::EGLint name) = 0;
    virtual egl::EGLImage eglCreateImageKHR(egl::EGLDisplay display, egl::EGLContext context, egl::EGLenum target,
                                            egl::EGLClientBuffer buffer, const egl::EGLint *attributes) = 0;
    virtual egl::EGLBoolean eglDestroyImageKHR(egl::EGLDisplay display, egl::EGLImage image) = 0;
    virtual egl::EGLBoolean eglQueryDmaBufModifiersEXT(egl::EGLDisplay display, egl::EGLint format,
                                                       egl::EGLint maxModifiers, egl::EGLuint64KHR *modifiers,
                                                       egl::EGLBoolean *externalOnly, egl::EGLint *numModifiers) = 0;
    virtual egl::EGLContext eglGetCurrentContext() = 0;
    virtual egl::EGLBoolean eglQueryContext(egl::EGLDisplay display, egl::EGLContext context, egl::EGLint attribute,
                                            egl::EGLint *value) = 0;
    virtual egl::EGLBoolean eglChooseConfig(egl::EGLDisplay display, const egl::EGLint *attributes,
                                            egl::EGLConfig *configs, egl::EGLint configSize, egl::EGLint *numConfigs) = 0;
    virtual egl::EGLBoolean eglBindAPI(egl::EGLenum api) = 0;
    virtual egl::EGLContext eglCreateContext(egl::EGLDisplay display, egl::EGLConfig config,
                                             egl::EGLContext shareContext, const egl::EGLint *attributes) = 0;
    virtual egl::EGLBoolean eglDestroyContext(egl::EGLDisplay display, egl::EGLContext context) = 0;
    virtual egl::EGLSurface eglCreatePbufferSurface(egl::EGLDisplay display, egl::EGLConfig config,
                                                    const egl::EGLint *attributes) = 0;
    virtual egl::EGLBoolean eglDestroySurface(egl::EGLDisplay display, egl::EGLSurface surface) = 0;
    virtual egl::EGLBoolean eglMakeCurrent(egl::EGLDisplay display, egl::EGLSurface draw, egl::EGLSurface read,
                                           egl::EGLContext context) = 0;
    virtual egl::EGLBoolean eglReleaseThread() = 0;

    // GL
    virtual const egl::GLubyte *glGetString(egl::GLenum name) = 0;
    virtual egl::GLenum glGetError() = 0;
    virtual void glGenTextures(egl::GLsizei count, egl::GLuint *textures) = 0;
    virtual void glBindTexture(egl::GLenum target, egl::GLuint texture) = 0;
    virtual void glTexParameteri(egl::GLenum target, egl::GLenum name, egl::GLint value) = 0;
    virtual void glEGLImageTargetTexture2DOES(egl::GLenum target, egl::GLeglImageOES image) = 0;
    virtual void glDeleteTextures(egl::GLsizei count, const egl::GLuint *textures) = 0;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_EGL_GL_H_
