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

#include "EglLibrary.h"
#include "RialtoClientLogging.h"

#include <dlfcn.h>

#include <cstdlib>
#include <type_traits>

namespace firebolt::rialto::client
{
namespace
{
using namespace egl;

/**
 * @brief Entry points resolved with eglGetProcAddress, falling back to the loaded EGL library for core EGL calls.
 */
class EglGl : public IEglGl
{
public:
    bool resolve(void *handle)
    {
        using GetProcAddress = void *(*)(const char *);
        auto getProcAddress = reinterpret_cast<GetProcAddress>(dlsym(handle, "eglGetProcAddress"));
        if (!getProcAddress)
        {
            RIALTO_CLIENT_LOG_ERROR("eglGetProcAddress is missing");
            return false;
        }
        bool ok{true};
        auto get = [&](auto &function, const char *name)
        {
            void *address = getProcAddress(name);
            if (!address)
                address = dlsym(handle, name);
            if (!address)
            {
                RIALTO_CLIENT_LOG_ERROR("EGL/GL entry point %s is missing", name);
                ok = false;
            }
            function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(address);
        };
        get(m_eglGetError, "eglGetError");
        get(m_eglQueryString, "eglQueryString");
        get(m_eglCreateImageKHR, "eglCreateImageKHR");
        get(m_eglDestroyImageKHR, "eglDestroyImageKHR");
        get(m_eglGetCurrentContext, "eglGetCurrentContext");
        get(m_eglQueryContext, "eglQueryContext");
        get(m_eglChooseConfig, "eglChooseConfig");
        get(m_eglBindAPI, "eglBindAPI");
        get(m_eglCreateContext, "eglCreateContext");
        get(m_eglDestroyContext, "eglDestroyContext");
        get(m_eglCreatePbufferSurface, "eglCreatePbufferSurface");
        get(m_eglDestroySurface, "eglDestroySurface");
        get(m_eglMakeCurrent, "eglMakeCurrent");
        get(m_eglReleaseThread, "eglReleaseThread");
        get(m_glGetString, "glGetString");
        get(m_glGetError, "glGetError");
        get(m_glGenTextures, "glGenTextures");
        get(m_glBindTexture, "glBindTexture");
        get(m_glTexParameteri, "glTexParameteri");
        get(m_glEGLImageTargetTexture2DOES, "glEGLImageTargetTexture2DOES");
        get(m_glDeleteTextures, "glDeleteTextures");
        // optional: only needed to check the format and modifier; a missing one fails that check instead
        m_eglQueryDmaBufModifiersEXT =
            reinterpret_cast<decltype(m_eglQueryDmaBufModifiersEXT)>(getProcAddress("eglQueryDmaBufModifiersEXT"));
        return ok;
    }

    EGLint eglGetError() override { return m_eglGetError(); }
    const char *eglQueryString(EGLDisplay display, EGLint name) override { return m_eglQueryString(display, name); }
    EGLImage eglCreateImageKHR(EGLDisplay display, EGLContext context, EGLenum target, EGLClientBuffer buffer,
                               const EGLint *attributes) override
    {
        return m_eglCreateImageKHR(display, context, target, buffer, attributes);
    }
    EGLBoolean eglDestroyImageKHR(EGLDisplay display, EGLImage image) override
    {
        return m_eglDestroyImageKHR(display, image);
    }
    EGLBoolean eglQueryDmaBufModifiersEXT(EGLDisplay display, EGLint format, EGLint maxModifiers,
                                          EGLuint64KHR *modifiers, EGLBoolean *externalOnly, EGLint *numModifiers) override
    {
        if (!m_eglQueryDmaBufModifiersEXT)
            return EGL_FALSE;
        return m_eglQueryDmaBufModifiersEXT(display, format, maxModifiers, modifiers, externalOnly, numModifiers);
    }
    EGLContext eglGetCurrentContext() override { return m_eglGetCurrentContext(); }
    EGLBoolean eglQueryContext(EGLDisplay display, EGLContext context, EGLint attribute, EGLint *value) override
    {
        return m_eglQueryContext(display, context, attribute, value);
    }
    EGLBoolean eglChooseConfig(EGLDisplay display, const EGLint *attributes, EGLConfig *configs, EGLint configSize,
                               EGLint *numConfigs) override
    {
        return m_eglChooseConfig(display, attributes, configs, configSize, numConfigs);
    }
    EGLBoolean eglBindAPI(EGLenum api) override { return m_eglBindAPI(api); }
    EGLContext eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext shareContext,
                                const EGLint *attributes) override
    {
        return m_eglCreateContext(display, config, shareContext, attributes);
    }
    EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context) override
    {
        return m_eglDestroyContext(display, context);
    }
    EGLSurface eglCreatePbufferSurface(EGLDisplay display, EGLConfig config, const EGLint *attributes) override
    {
        return m_eglCreatePbufferSurface(display, config, attributes);
    }
    EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface) override
    {
        return m_eglDestroySurface(display, surface);
    }
    EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context) override
    {
        return m_eglMakeCurrent(display, draw, read, context);
    }
    EGLBoolean eglReleaseThread() override { return m_eglReleaseThread(); }
    const GLubyte *glGetString(GLenum name) override { return m_glGetString(name); }
    GLenum glGetError() override { return m_glGetError(); }
    void glGenTextures(GLsizei count, GLuint *textures) override { m_glGenTextures(count, textures); }
    void glBindTexture(GLenum target, GLuint texture) override { m_glBindTexture(target, texture); }
    void glTexParameteri(GLenum target, GLenum name, GLint value) override { m_glTexParameteri(target, name, value); }
    void glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) override
    {
        m_glEGLImageTargetTexture2DOES(target, image);
    }
    void glDeleteTextures(GLsizei count, const GLuint *textures) override { m_glDeleteTextures(count, textures); }

private:
    EGLint (*m_eglGetError)(){nullptr};
    const char *(*m_eglQueryString)(EGLDisplay, EGLint){nullptr};
    EGLImage (*m_eglCreateImageKHR)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint *){nullptr};
    EGLBoolean (*m_eglDestroyImageKHR)(EGLDisplay, EGLImage){nullptr};
    EGLBoolean (*m_eglQueryDmaBufModifiersEXT)(EGLDisplay, EGLint, EGLint, EGLuint64KHR *, EGLBoolean *,
                                               EGLint *){nullptr};
    EGLContext (*m_eglGetCurrentContext)(){nullptr};
    EGLBoolean (*m_eglQueryContext)(EGLDisplay, EGLContext, EGLint, EGLint *){nullptr};
    EGLBoolean (*m_eglChooseConfig)(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *){nullptr};
    EGLBoolean (*m_eglBindAPI)(EGLenum){nullptr};
    EGLContext (*m_eglCreateContext)(EGLDisplay, EGLConfig, EGLContext, const EGLint *){nullptr};
    EGLBoolean (*m_eglDestroyContext)(EGLDisplay, EGLContext){nullptr};
    EGLSurface (*m_eglCreatePbufferSurface)(EGLDisplay, EGLConfig, const EGLint *){nullptr};
    EGLBoolean (*m_eglDestroySurface)(EGLDisplay, EGLSurface){nullptr};
    EGLBoolean (*m_eglMakeCurrent)(EGLDisplay, EGLSurface, EGLSurface, EGLContext){nullptr};
    EGLBoolean (*m_eglReleaseThread)(){nullptr};
    const GLubyte *(*m_glGetString)(GLenum){nullptr};
    GLenum (*m_glGetError)(){nullptr};
    void (*m_glGenTextures)(GLsizei, GLuint *){nullptr};
    void (*m_glBindTexture)(GLenum, GLuint){nullptr};
    void (*m_glTexParameteri)(GLenum, GLenum, GLint){nullptr};
    void (*m_glEGLImageTargetTexture2DOES)(GLenum, GLeglImageOES){nullptr};
    void (*m_glDeleteTextures)(GLsizei, const GLuint *){nullptr};
};
} // namespace

std::shared_ptr<IEglLibrary> IEglLibrary::instance()
{
    static std::shared_ptr<IEglLibrary> library{std::make_shared<EglLibrary>()};
    return library;
}

EglLibrary::~EglLibrary()
{
    if (m_handle)
    {
        dlclose(m_handle);
    }
}

bool EglLibrary::load()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!m_loadAttempted)
    {
        m_loadAttempted = true;
        // RIALTO_EGL_LIBRARY overrides the library, for tests and for platforms that name it differently
        const char *kOverride = std::getenv(kLibraryEnv);
        const char *kName = (kOverride && *kOverride) ? kOverride : kLibraryName;
        m_handle = dlopen(kName, RTLD_NOW | RTLD_LOCAL);
        if (!m_handle)
        {
            const char *kError = dlerror();
            RIALTO_CLIENT_LOG_WARN("Cannot load %s: %s", kName, kError ? kError : "unknown error");
        }
    }
    return m_handle != nullptr;
}

std::shared_ptr<IEglGl> EglLibrary::functions()
{
    if (!load())
    {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!m_resolveAttempted)
    {
        m_resolveAttempted = true;
        auto functions = std::make_shared<EglGl>();
        if (functions->resolve(m_handle))
        {
            m_functions = functions;
        }
    }
    return m_functions;
}
} // namespace firebolt::rialto::client
