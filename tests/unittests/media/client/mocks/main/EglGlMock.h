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

#ifndef FIREBOLT_RIALTO_CLIENT_EGL_GL_MOCK_H_
#define FIREBOLT_RIALTO_CLIENT_EGL_GL_MOCK_H_

#include "IEglGl.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::client
{
using namespace egl;

class EglGlMock : public IEglGl
{
public:
    MOCK_METHOD(EGLint, eglGetError, (), (override));
    MOCK_METHOD(const char *, eglQueryString, (EGLDisplay display, EGLint name), (override));
    MOCK_METHOD(EGLImage, eglCreateImageKHR,
                (EGLDisplay display, EGLContext context, EGLenum target, EGLClientBuffer buffer, const EGLint *attributes),
                (override));
    MOCK_METHOD(EGLBoolean, eglDestroyImageKHR, (EGLDisplay display, EGLImage image), (override));
    MOCK_METHOD(EGLBoolean, eglQueryDmaBufModifiersEXT,
                (EGLDisplay display, EGLint format, EGLint maxModifiers, EGLuint64KHR *modifiers,
                 EGLBoolean *externalOnly, EGLint *numModifiers),
                (override));
    MOCK_METHOD(EGLContext, eglGetCurrentContext, (), (override));
    MOCK_METHOD(EGLBoolean, eglQueryContext, (EGLDisplay display, EGLContext context, EGLint attribute, EGLint *value),
                (override));
    MOCK_METHOD(EGLBoolean, eglChooseConfig,
                (EGLDisplay display, const EGLint *attributes, EGLConfig *configs, EGLint configSize, EGLint *numConfigs),
                (override));
    MOCK_METHOD(EGLBoolean, eglBindAPI, (EGLenum api), (override));
    MOCK_METHOD(EGLContext, eglCreateContext,
                (EGLDisplay display, EGLConfig config, EGLContext shareContext, const EGLint *attributes), (override));
    MOCK_METHOD(EGLBoolean, eglDestroyContext, (EGLDisplay display, EGLContext context), (override));
    MOCK_METHOD(EGLSurface, eglCreatePbufferSurface, (EGLDisplay display, EGLConfig config, const EGLint *attributes),
                (override));
    MOCK_METHOD(EGLBoolean, eglDestroySurface, (EGLDisplay display, EGLSurface surface), (override));
    MOCK_METHOD(EGLBoolean, eglMakeCurrent, (EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context),
                (override));
    MOCK_METHOD(EGLBoolean, eglReleaseThread, (), (override));
    MOCK_METHOD(const GLubyte *, glGetString, (GLenum name), (override));
    MOCK_METHOD(GLenum, glGetError, (), (override));
    MOCK_METHOD(void, glGenTextures, (GLsizei count, GLuint *textures), (override));
    MOCK_METHOD(void, glBindTexture, (GLenum target, GLuint texture), (override));
    MOCK_METHOD(void, glTexParameteri, (GLenum target, GLenum name, GLint value), (override));
    MOCK_METHOD(void, glEGLImageTargetTexture2DOES, (GLenum target, GLeglImageOES image), (override));
    MOCK_METHOD(void, glDeleteTextures, (GLsizei count, const GLuint *textures), (override));
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_EGL_GL_MOCK_H_
