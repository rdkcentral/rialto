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

#include "CaptureCore.h"
#include "GlFrameImporterTestBase.h"
#include "VideoFrameCaptureIpcMock.h"

#include <fcntl.h>
#include <set>
#include <thread>

/**
 * The EGL/GL mock keeps live images and textures, so a test can check that nothing from a failed attempt remains.
 */
class GlFrameImporterImportTest : public GlFrameImporterTestBase
{
protected:
    void SetUp() override
    {
        displayHasExtensions();
        displayListsModifiers({kLinear});
        ON_CALL(*m_gl, eglCreateImageKHR(m_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, _))
            .WillByDefault(Invoke(
                [this](EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint *attributes) -> EGLImage
                {
                    m_lastAttributes.clear();
                    for (const EGLint *it = attributes; *it != EGL_NONE; ++it)
                        m_lastAttributes.push_back(*it);
                    if (m_failImage)
                        return EGL_NO_IMAGE;
                    m_images.insert(++m_nextHandle);
                    return reinterpret_cast<EGLImage>(m_nextHandle);
                }));
        ON_CALL(*m_gl, eglDestroyImageKHR(m_display, _))
            .WillByDefault(
                Invoke([this](EGLDisplay, EGLImage image)
                       { return m_images.erase(reinterpret_cast<std::uintptr_t>(image)) ? EGL_TRUE : EGL_FALSE; }));
        ON_CALL(*m_gl, glGenTextures(1, _))
            .WillByDefault(Invoke(
                [this](GLsizei, GLuint *textures)
                {
                    *textures = m_failTexture ? 0 : ++m_nextTexture;
                    if (*textures)
                        m_textures.insert(*textures);
                }));
        ON_CALL(*m_gl, glDeleteTextures(1, _))
            .WillByDefault(Invoke([this](GLsizei, const GLuint *textures) { m_textures.erase(*textures); }));
        ON_CALL(*m_gl, glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, _))
            .WillByDefault(Invoke([this](GLenum, GLeglImageOES) { m_glError = m_failTarget ? 0x0502 : GL_NO_ERROR; }));
        ON_CALL(*m_gl, glGetError())
            .WillByDefault(Invoke(
                [this]()
                {
                    const GLenum kError{m_glError};
                    m_glError = GL_NO_ERROR;
                    return kError;
                }));
    }

    CaptureFrameDescription exampleB()
    {
        CaptureFrameDescription description;
        description.slotIndex = 0;
        description.drmFormat = kNv12;
        description.backingWidth = 64;
        description.backingHeight = 32;
        description.objects = {{101, 4096}};
        description.planes = {{0, 101, 0, 2048, 64}, {1, 101, 2048, 1024, 64}};
        return description;
    }

    GlFrameImporter m_importer{m_gl, m_display, m_context};
    std::set<std::uintptr_t> m_images;
    std::set<GLuint> m_textures;
    std::uintptr_t m_nextHandle{0x100};
    GLuint m_nextTexture{0};
    GLenum m_glError{GL_NO_ERROR};
    bool m_failImage{false};
    bool m_failTexture{false};
    bool m_failTarget{false};
    std::vector<EGLint> m_lastAttributes;
};

TEST_F(GlFrameImporterImportTest, ImportGivesOneExternalTextureAndBorrowsDescriptors)
{
    bool fdsConsumed{true};
    EXPECT_CALL(*m_gl, glBindTexture(GL_TEXTURE_EXTERNAL_OES, _)).Times(2); // bind, then unbind
    EXPECT_CALL(*m_gl, glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, _));
    auto image = m_importer.importFrame(exampleB(), {{101, 10}}, fdsConsumed);
    ASSERT_NE(image, nullptr);
    EXPECT_FALSE(fdsConsumed);
    EXPECT_EQ(image->textureTarget(), GL_TEXTURE_EXTERNAL_OES);
    ASSERT_EQ(image->textureIds().size(), 1U);
    EXPECT_EQ(m_textures.count(image->textureIds()[0]), 1U);
    EXPECT_EQ(m_images.size(), 1U);
    EXPECT_FALSE(m_lastAttributes.empty()); // built from the description
}

TEST_F(GlFrameImporterImportTest, ImageFailureLeavesNothing)
{
    m_failImage = true;
    bool fdsConsumed{true};
    EXPECT_CALL(*m_gl, glGenTextures(_, _)).Times(0);
    EXPECT_EQ(m_importer.importFrame(exampleB(), {{101, 10}}, fdsConsumed), nullptr);
    EXPECT_FALSE(fdsConsumed);
    EXPECT_TRUE(m_images.empty());
    EXPECT_TRUE(m_textures.empty());
}

TEST_F(GlFrameImporterImportTest, TextureCreationFailureLeavesNothing)
{
    m_failTexture = true;
    bool fdsConsumed{true};
    EXPECT_EQ(m_importer.importFrame(exampleB(), {{101, 10}}, fdsConsumed), nullptr);
    EXPECT_TRUE(m_images.empty()); // the image was destroyed again
    EXPECT_TRUE(m_textures.empty());
}

TEST_F(GlFrameImporterImportTest, TextureBindingFailureLeavesNothing)
{
    m_failTarget = true;
    bool fdsConsumed{true};
    EXPECT_EQ(m_importer.importFrame(exampleB(), {{101, 10}}, fdsConsumed), nullptr);
    EXPECT_TRUE(m_images.empty());
    EXPECT_TRUE(m_textures.empty());
}

TEST_F(GlFrameImporterImportTest, EarlierApplicationGlErrorDoesNotFailTheImport)
{
    m_glError = 0x0500; // GL_INVALID_ENUM left over from the application
    bool fdsConsumed{true};
    EXPECT_NE(m_importer.importFrame(exampleB(), {{101, 10}}, fdsConsumed), nullptr);
}

/**
 * Through the capture: every received descriptor is closed after a successful and after a failed import.
 */
TEST_F(GlFrameImporterImportTest, DescriptorsAreClosedOnSuccessAndFailure)
{
    auto ipc = std::make_shared<NiceMock<VideoFrameCaptureIpcMock>>();
    auto importer = std::make_shared<GlFrameImporter>(m_gl, m_display, m_context);
    ON_CALL(*ipc, releaseFrame(_, _, _, _)).WillByDefault(Return(true));
    std::vector<int> fds;
    ON_CALL(*ipc, acquireCurrentFrame(5))
        .WillByDefault(Invoke(
            [&](std::uint32_t)
            {
                CaptureAcquireResult result;
                result.status = CaptureAcquireStatus::OK;
                result.description = exampleB();
                result.description.slotIndex = static_cast<std::uint32_t>(fds.size());
                result.description.acquisitionToken = fds.size() + 1;
                result.description.dmaBufObjectsFlushed = true; // no cache hit: import every time
                const int kFd{memfd_create("gl-import-test", MFD_CLOEXEC)};
                fds.push_back(kFd);
                result.fds = {{101, kFd}};
                return result;
            }));
    {
        CaptureCore core{ipc, 5, importer};
        EXPECT_NE(core.acquireCurrentFrame().frame, nullptr); // success
        m_failImage = true;
        EXPECT_FALSE(core.acquireCurrentFrame().changed); // failure
    }
    ASSERT_EQ(fds.size(), 2U);
    for (int fd : fds)
        EXPECT_EQ(fcntl(fd, F_GETFD), -1);
}

TEST_F(GlFrameImporterImportTest, GlFramesExposeOneExternalTextureWithRgbaLayout)
{
    auto ipc = std::make_shared<NiceMock<VideoFrameCaptureIpcMock>>();
    ON_CALL(*ipc, releaseFrame(_, _, _, _)).WillByDefault(Return(true));
    const int kFd{memfd_create("gl-frame-test", MFD_CLOEXEC)};
    ON_CALL(*ipc, acquireCurrentFrame(5))
        .WillByDefault(Invoke(
            [&](std::uint32_t)
            {
                CaptureAcquireResult result;
                result.status = CaptureAcquireStatus::OK;
                result.description = exampleB(); // NV12
                result.description.acquisitionToken = 1;
                result.fds = {{101, kFd}};
                return result;
            }));
    CaptureCore core{ipc, 5, std::make_shared<GlFrameImporter>(m_gl, m_display, m_context)};

    const std::shared_ptr<IGLVideoFrame> kFrame{core.acquireCurrentFrame().frame};
    ASSERT_NE(kFrame, nullptr);
    EXPECT_EQ(kFrame->textureTarget(), GL_TEXTURE_EXTERNAL_OES);
    ASSERT_EQ(kFrame->textureIds().size(), 1U);
    EXPECT_EQ(m_textures.count(kFrame->textureIds()[0]), 1U);
    EXPECT_EQ(kFrame->layout(), GraphicsTextureLayout::RGBA);
    EXPECT_EQ(kFrame->numResources(), 1U);
}

class GlFrameImporterTeardownTest : public GlFrameImporterImportTest
{
protected:
    // An importer with one import retired, as it is when the capture and its last frame are gone.
    std::unique_ptr<GlFrameImporter> importerWithRetiredImport()
    {
        auto importer = std::make_unique<GlFrameImporter>(m_gl, m_display, m_context);
        bool fdsConsumed{false};
        auto image = importer->importFrame(exampleB(), {{101, 10}}, fdsConsumed);
        EXPECT_NE(image, nullptr);
        importer->retire(image);
        return importer;
    }

    void deletionsHappenOn(std::thread::id &thread)
    {
        ON_CALL(*m_gl, glDeleteTextures(1, _))
            .WillByDefault(Invoke(
                [this, &thread](GLsizei, const GLuint *textures)
                {
                    thread = std::this_thread::get_id();
                    m_textures.erase(*textures);
                }));
    }

    void contextCreationWorks()
    {
        ON_CALL(*m_gl, eglBindAPI(EGL_OPENGL_ES_API)).WillByDefault(Return(EGL_TRUE));
        ON_CALL(*m_gl, eglQueryContext(m_display, m_context, _, _))
            .WillByDefault(DoAll(SetArgPointee<3>(2), Return(EGL_TRUE)));
        ON_CALL(*m_gl, eglChooseConfig(m_display, _, _, 1, _))
            .WillByDefault(Invoke(
                [](EGLDisplay, const EGLint *, EGLConfig *configs, EGLint, EGLint *count)
                {
                    configs[0] = reinterpret_cast<EGLConfig>(0xCF);
                    *count = 1;
                    return EGL_TRUE;
                }));
        ON_CALL(*m_gl, eglMakeCurrent(m_display, _, _, _)).WillByDefault(Return(EGL_TRUE));
    }

    EGLContext m_temporary{reinterpret_cast<EGLContext>(0xC2)};
};

TEST_F(GlFrameImporterTeardownTest, DeletesOnThisThreadWhenTheCreationContextIsCurrent)
{
    auto importer = importerWithRetiredImport();
    std::thread::id deletingThread;
    deletionsHappenOn(deletingThread);
    EXPECT_CALL(*m_gl, eglGetCurrentContext()).WillOnce(Return(m_context));
    EXPECT_CALL(*m_gl, eglCreateContext(_, _, _, _)).Times(0);

    importer.reset();
    EXPECT_EQ(deletingThread, std::this_thread::get_id());
    EXPECT_TRUE(m_textures.empty());
    EXPECT_TRUE(m_images.empty());
}

TEST_F(GlFrameImporterTeardownTest, DeletesOnATemporarySurfacelessContextFromAnotherThread)
{
    displayHasExtensions("EGL_EXT_image_dma_buf_import EGL_EXT_image_dma_buf_import_modifiers "
                         "EGL_KHR_surfaceless_context");
    auto importer = importerWithRetiredImport();
    std::thread::id deletingThread;
    deletionsHappenOn(deletingThread);
    contextCreationWorks();
    EXPECT_CALL(*m_gl, eglGetCurrentContext()).WillOnce(Return(EGL_NO_CONTEXT));
    EXPECT_CALL(*m_gl, eglCreateContext(m_display, reinterpret_cast<EGLConfig>(0xCF), m_context, _))
        .WillOnce(Return(m_temporary));
    EXPECT_CALL(*m_gl, eglCreatePbufferSurface(_, _, _)).Times(0);
    EXPECT_CALL(*m_gl, eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, m_temporary)).WillOnce(Return(EGL_TRUE));
    EXPECT_CALL(*m_gl, eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)).WillOnce(Return(EGL_TRUE));
    EXPECT_CALL(*m_gl, eglDestroyContext(m_display, m_temporary)).WillOnce(Return(EGL_TRUE));
    EXPECT_CALL(*m_gl, eglReleaseThread()).WillOnce(Return(EGL_TRUE));

    importer.reset(); // returns only after the thread has finished
    EXPECT_NE(deletingThread, std::thread::id{});
    EXPECT_NE(deletingThread, std::this_thread::get_id());
    EXPECT_TRUE(m_textures.empty());
    EXPECT_TRUE(m_images.empty());
}

TEST_F(GlFrameImporterTeardownTest, UsesAOnePixelPbufferWithoutSurfacelessContexts)
{
    auto importer = importerWithRetiredImport(); // the default extensions have no surfaceless contexts
    contextCreationWorks();
    const EGLSurface kPbuffer{reinterpret_cast<EGLSurface>(0x5F)};
    EXPECT_CALL(*m_gl, eglGetCurrentContext()).WillOnce(Return(EGL_NO_CONTEXT));
    EXPECT_CALL(*m_gl, eglCreateContext(m_display, _, m_context, _)).WillOnce(Return(m_temporary));
    EXPECT_CALL(*m_gl, eglCreatePbufferSurface(m_display, reinterpret_cast<EGLConfig>(0xCF), _))
        .WillOnce(Invoke(
            [kPbuffer](EGLDisplay, EGLConfig, const EGLint *attributes)
            {
                EXPECT_EQ(attributes[0], EGL_WIDTH);
                EXPECT_EQ(attributes[1], 1);
                EXPECT_EQ(attributes[2], EGL_HEIGHT);
                EXPECT_EQ(attributes[3], 1);
                return kPbuffer;
            }));
    EXPECT_CALL(*m_gl, eglMakeCurrent(m_display, kPbuffer, kPbuffer, m_temporary)).WillOnce(Return(EGL_TRUE));
    EXPECT_CALL(*m_gl, eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)).WillOnce(Return(EGL_TRUE));
    EXPECT_CALL(*m_gl, eglDestroySurface(m_display, kPbuffer)).WillOnce(Return(EGL_TRUE));
    EXPECT_CALL(*m_gl, eglDestroyContext(m_display, m_temporary)).WillOnce(Return(EGL_TRUE));

    importer.reset();
    EXPECT_TRUE(m_textures.empty());
    EXPECT_TRUE(m_images.empty());
}

TEST_F(GlFrameImporterTeardownTest, LogsAndKeepsObjectsWhenNoTemporaryContextCanBeCreated)
{
    auto importer = importerWithRetiredImport();
    contextCreationWorks();
    EXPECT_CALL(*m_gl, eglGetCurrentContext()).WillOnce(Return(EGL_NO_CONTEXT));
    EXPECT_CALL(*m_gl, eglCreateContext(_, _, _, _)).WillOnce(Return(EGL_NO_CONTEXT));
    ClientErrorLog log;
    importer.reset();
    EXPECT_EQ(log.count("no temporary context"), 1U);
    EXPECT_EQ(m_textures.size(), 1U); // left to the share group
}

TEST_F(GlFrameImporterTeardownTest, NothingToDeleteMakesNoCalls)
{
    auto importer = std::make_unique<GlFrameImporter>(m_gl, m_display, m_context);
    EXPECT_CALL(*m_gl, eglGetCurrentContext()).Times(0);
    importer.reset();
}
