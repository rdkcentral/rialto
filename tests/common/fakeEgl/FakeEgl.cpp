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

// A fake libEGL for tests: exports the EGL and GL entry points Rialto resolves, keeps counts of live objects, and
// tracks the current context per thread. It renders nothing.

#include "FakeEglControl.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <set>
#include <string>

namespace
{
using EGLBoolean = unsigned int;
using EGLint = std::int32_t;
using EGLenum = unsigned int;
using GLuint = unsigned int;
using GLenum = unsigned int;
using GLint = int;
using GLsizei = int;

constexpr EGLint kEglExtensions{0x3055};
constexpr EGLint kEglConfigId{0x3028};
constexpr EGLint kEglContextClientVersion{0x3098};
constexpr GLenum kGlExtensions{0x1F03};

std::mutex g_mutex;
std::set<std::uintptr_t> g_images;
std::set<GLuint> g_textures;
std::set<std::uintptr_t> g_contexts;
std::uintptr_t g_nextHandle{0x1000};
GLuint g_nextTexture{1};
std::atomic<bool> g_failImages{false};
std::string g_extensions{"EGL_EXT_image_dma_buf_import EGL_EXT_image_dma_buf_import_modifiers "
                         "EGL_KHR_surfaceless_context"};
thread_local void *t_currentContext{nullptr};
thread_local int t_calls{0};

void countCall()
{
    ++t_calls;
}
} // namespace

extern "C"
{
    void fakeEglReset()
    {
        std::lock_guard<std::mutex> lock{g_mutex};
        g_images.clear();
        g_textures.clear();
        g_contexts.clear();
        g_failImages = false;
        g_extensions = "EGL_EXT_image_dma_buf_import EGL_EXT_image_dma_buf_import_modifiers "
                       "EGL_KHR_surfaceless_context";
        t_calls = 0;
    }
    int fakeEglLiveImages()
    {
        std::lock_guard<std::mutex> lock{g_mutex};
        return static_cast<int>(g_images.size());
    }
    int fakeEglLiveTextures()
    {
        std::lock_guard<std::mutex> lock{g_mutex};
        return static_cast<int>(g_textures.size());
    }
    int fakeEglLiveContexts()
    {
        std::lock_guard<std::mutex> lock{g_mutex};
        return static_cast<int>(g_contexts.size());
    }
    int fakeEglThreadCallCount()
    {
        return t_calls;
    }
    void fakeEglResetThreadCallCount()
    {
        t_calls = 0;
    }
    void fakeEglFailImages(bool fail)
    {
        g_failImages = fail;
    }
    void fakeEglSetExtensions(const char *extensions)
    {
        std::lock_guard<std::mutex> lock{g_mutex};
        g_extensions = extensions ? extensions : "";
    }

    // EGL
    EGLint eglGetError()
    {
        countCall();
        return 0x3000;
    }
    const char *eglQueryString(void *, EGLint name)
    {
        countCall();
        static thread_local std::string copy;
        std::lock_guard<std::mutex> lock{g_mutex};
        copy = (name == kEglExtensions) ? g_extensions : std::string{"1.4 fake"};
        return copy.c_str();
    }
    void *eglCreateImageKHR(void *, void *, EGLenum, void *, const EGLint *)
    {
        countCall();
        if (g_failImages)
            return nullptr;
        std::lock_guard<std::mutex> lock{g_mutex};
        const std::uintptr_t kImage{g_nextHandle++};
        g_images.insert(kImage);
        return reinterpret_cast<void *>(kImage);
    }
    EGLBoolean eglDestroyImageKHR(void *, void *image)
    {
        countCall();
        std::lock_guard<std::mutex> lock{g_mutex};
        return g_images.erase(reinterpret_cast<std::uintptr_t>(image)) ? 1 : 0;
    }
    EGLBoolean eglQueryDmaBufModifiersEXT(void *, EGLint, EGLint maxModifiers, std::uint64_t *modifiers,
                                          EGLBoolean *externalOnly, EGLint *numModifiers)
    {
        countCall();
        // every format supports the linear modifier only
        if (maxModifiers > 0 && modifiers)
        {
            modifiers[0] = 0;
            if (externalOnly)
                externalOnly[0] = 1;
        }
        if (numModifiers)
            *numModifiers = 1;
        return 1;
    }
    void *eglGetCurrentContext()
    {
        countCall();
        return t_currentContext;
    }
    EGLBoolean eglQueryContext(void *, void *, EGLint attribute, EGLint *value)
    {
        countCall();
        if (value)
            *value = (attribute == kEglContextClientVersion) ? 2 : (attribute == kEglConfigId ? 1 : 0);
        return 1;
    }
    EGLBoolean eglChooseConfig(void *, const EGLint *, void **configs, EGLint configSize, EGLint *numConfigs)
    {
        countCall();
        if (configs && configSize > 0)
            configs[0] = reinterpret_cast<void *>(0x1);
        if (numConfigs)
            *numConfigs = 1;
        return 1;
    }
    EGLBoolean eglBindAPI(EGLenum)
    {
        countCall();
        return 1;
    }
    void *eglCreateContext(void *, void *, void *, const EGLint *)
    {
        countCall();
        std::lock_guard<std::mutex> lock{g_mutex};
        const std::uintptr_t kContext{g_nextHandle++};
        g_contexts.insert(kContext);
        return reinterpret_cast<void *>(kContext);
    }
    EGLBoolean eglDestroyContext(void *, void *context)
    {
        countCall();
        std::lock_guard<std::mutex> lock{g_mutex};
        return g_contexts.erase(reinterpret_cast<std::uintptr_t>(context)) ? 1 : 0;
    }
    void *eglCreatePbufferSurface(void *, void *, const EGLint *)
    {
        countCall();
        return reinterpret_cast<void *>(0x2);
    }
    EGLBoolean eglDestroySurface(void *, void *)
    {
        countCall();
        return 1;
    }
    EGLBoolean eglMakeCurrent(void *, void *, void *, void *context)
    {
        countCall();
        t_currentContext = context;
        return 1;
    }
    EGLBoolean eglReleaseThread()
    {
        countCall();
        t_currentContext = nullptr;
        return 1;
    }

    // GL
    const unsigned char *glGetString(GLenum name)
    {
        countCall();
        static const char kExtensions[]{"GL_OES_EGL_image_external"};
        static const char kOther[]{"fake"};
        return reinterpret_cast<const unsigned char *>(name == kGlExtensions ? kExtensions : kOther);
    }
    GLenum glGetError()
    {
        countCall();
        return 0;
    }
    void glGenTextures(GLsizei count, GLuint *textures)
    {
        countCall();
        std::lock_guard<std::mutex> lock{g_mutex};
        for (GLsizei i = 0; i < count; ++i)
        {
            textures[i] = g_nextTexture++;
            g_textures.insert(textures[i]);
        }
    }
    void glBindTexture(GLenum, GLuint)
    {
        countCall();
    }
    void glTexParameteri(GLenum, GLenum, GLint)
    {
        countCall();
    }
    void glEGLImageTargetTexture2DOES(GLenum, void *)
    {
        countCall();
    }
    void glDeleteTextures(GLsizei count, const GLuint *textures)
    {
        countCall();
        std::lock_guard<std::mutex> lock{g_mutex};
        for (GLsizei i = 0; i < count; ++i)
            g_textures.erase(textures[i]);
    }

    void *eglGetProcAddress(const char *name)
    {
        // every entry point is exported, so resolve it from this library
        struct Entry
        {
            const char *name;
            void *address;
        };
        static const Entry kEntries[]{
            {"eglGetError", reinterpret_cast<void *>(&eglGetError)},
            {"eglQueryString", reinterpret_cast<void *>(&eglQueryString)},
            {"eglCreateImageKHR", reinterpret_cast<void *>(&eglCreateImageKHR)},
            {"eglDestroyImageKHR", reinterpret_cast<void *>(&eglDestroyImageKHR)},
            {"eglQueryDmaBufModifiersEXT", reinterpret_cast<void *>(&eglQueryDmaBufModifiersEXT)},
            {"eglGetCurrentContext", reinterpret_cast<void *>(&eglGetCurrentContext)},
            {"eglQueryContext", reinterpret_cast<void *>(&eglQueryContext)},
            {"eglChooseConfig", reinterpret_cast<void *>(&eglChooseConfig)},
            {"eglBindAPI", reinterpret_cast<void *>(&eglBindAPI)},
            {"eglCreateContext", reinterpret_cast<void *>(&eglCreateContext)},
            {"eglDestroyContext", reinterpret_cast<void *>(&eglDestroyContext)},
            {"eglCreatePbufferSurface", reinterpret_cast<void *>(&eglCreatePbufferSurface)},
            {"eglDestroySurface", reinterpret_cast<void *>(&eglDestroySurface)},
            {"eglMakeCurrent", reinterpret_cast<void *>(&eglMakeCurrent)},
            {"eglReleaseThread", reinterpret_cast<void *>(&eglReleaseThread)},
            {"glGetString", reinterpret_cast<void *>(&glGetString)},
            {"glGetError", reinterpret_cast<void *>(&glGetError)},
            {"glGenTextures", reinterpret_cast<void *>(&glGenTextures)},
            {"glBindTexture", reinterpret_cast<void *>(&glBindTexture)},
            {"glTexParameteri", reinterpret_cast<void *>(&glTexParameteri)},
            {"glEGLImageTargetTexture2DOES", reinterpret_cast<void *>(&glEGLImageTargetTexture2DOES)},
            {"glDeleteTextures", reinterpret_cast<void *>(&glDeleteTextures)},
        };
        for (const Entry &kEntry : kEntries)
        {
            if (std::strcmp(kEntry.name, name) == 0)
                return kEntry.address;
        }
        return nullptr;
    }
}
