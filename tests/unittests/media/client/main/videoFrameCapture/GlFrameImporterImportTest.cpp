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
