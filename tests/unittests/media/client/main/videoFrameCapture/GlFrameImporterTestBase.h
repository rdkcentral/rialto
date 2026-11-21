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

#ifndef GL_FRAME_IMPORTER_TEST_BASE_H_
#define GL_FRAME_IMPORTER_TEST_BASE_H_

#include "EglGlMock.h"
#include "GlFrameImporter.h"
#include "RialtoLogging.h"

#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::client::egl;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

/** Counts client error logs while it exists. */
class ClientErrorLog
{
public:
    ClientErrorLog()
    {
        logging::setLogHandler(
            RIALTO_COMPONENT_CLIENT,
            [this](RIALTO_DEBUG_LEVEL level, const char *, int, const char *, const char *message, std::size_t length)
            {
                if (level == RIALTO_DEBUG_LEVEL_ERROR)
                {
                    std::lock_guard<std::mutex> lock{m_mutex};
                    m_errors.emplace_back(message, length);
                }
            },
            true);
    }
    ~ClientErrorLog() { logging::setLogHandler(RIALTO_COMPONENT_CLIENT, nullptr, false); }

    std::size_t count(const std::string &text)
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        std::size_t matches{0};
        for (const auto &kError : m_errors)
        {
            if (kError.find(text) != std::string::npos)
                ++matches;
        }
        return matches;
    }

private:
    std::mutex m_mutex;
    std::vector<std::string> m_errors;
};

class GlFrameImporterTestBase : public ::testing::Test
{
protected:
    static constexpr std::uint32_t kNv12{0x3231564e};
    static constexpr std::uint64_t kLinear{0};
    static constexpr std::uint64_t kTiled{0x0100000000000001};
    EGLDisplay m_display{reinterpret_cast<EGLDisplay>(0xD1)};
    EGLContext m_context{reinterpret_cast<EGLContext>(0xC1)};

    void displayHasExtensions(const char *eglExtensions = "EGL_EXT_image_dma_buf_import "
                                                          "EGL_EXT_image_dma_buf_import_modifiers",
                              const char *glExtensions = "GL_OES_EGL_image_external")
    {
        m_eglExtensions = eglExtensions;
        m_glExtensions = glExtensions;
        ON_CALL(*m_gl, eglQueryString(m_display, EGL_EXTENSIONS)).WillByDefault(Return(m_eglExtensions.c_str()));
        ON_CALL(*m_gl, glGetString(GL_EXTENSIONS))
            .WillByDefault(Return(reinterpret_cast<const GLubyte *>(m_glExtensions.c_str())));
    }

    // The display lists these modifiers for every format.
    void displayListsModifiers(const std::vector<EGLuint64KHR> &modifiers)
    {
        m_modifiers = modifiers;
        ON_CALL(*m_gl, eglQueryDmaBufModifiersEXT(m_display, _, _, _, _, _))
            .WillByDefault(Invoke(
                [this](EGLDisplay, EGLint, EGLint maxModifiers, EGLuint64KHR *modifiers, EGLBoolean *externalOnly,
                       EGLint *numModifiers)
                {
                    const EGLint kAvailable{static_cast<EGLint>(m_modifiers.size())};
                    for (EGLint i = 0; modifiers && i < maxModifiers && i < kAvailable; ++i)
                    {
                        modifiers[i] = m_modifiers[i];
                        if (externalOnly)
                            externalOnly[i] = EGL_TRUE;
                    }
                    *numModifiers = maxModifiers == 0 ? kAvailable : std::min(maxModifiers, kAvailable);
                    return EGL_TRUE;
                }));
    }

    std::shared_ptr<NiceMock<EglGlMock>> m_gl{std::make_shared<NiceMock<EglGlMock>>()};
    std::string m_eglExtensions;
    std::string m_glExtensions;
    std::vector<EGLuint64KHR> m_modifiers;
};

#endif // GL_FRAME_IMPORTER_TEST_BASE_H_
