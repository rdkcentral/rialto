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

class GlFrameImporterSupportTest : public GlFrameImporterTestBase
{
protected:
    GlFrameImporter m_importer{m_gl, m_display, m_context};
};

TEST_F(GlFrameImporterSupportTest, ListedPairPasses)
{
    displayHasExtensions();
    displayListsModifiers({kLinear, kTiled});
    EXPECT_TRUE(m_importer.isSupported(kNv12, kTiled));
    EXPECT_TRUE(m_importer.isSupported(kNv12, kLinear));
}

TEST_F(GlFrameImporterSupportTest, UnlistedModifierFailsAndIsLoggedOnce)
{
    displayHasExtensions();
    displayListsModifiers({kLinear});
    ClientErrorLog log;
    EXPECT_FALSE(m_importer.isSupported(kNv12, kTiled));
    EXPECT_FALSE(m_importer.isSupported(kNv12, kTiled));
    EXPECT_EQ(log.count("cannot import format"), 1U);
}

TEST_F(GlFrameImporterSupportTest, ExtensionsAreCheckedOncePerCapture)
{
    displayHasExtensions();
    displayListsModifiers({kLinear});
    EXPECT_CALL(*m_gl, eglQueryString(m_display, EGL_EXTENSIONS)).Times(1);
    EXPECT_CALL(*m_gl, glGetString(GL_EXTENSIONS)).Times(1);
    EXPECT_TRUE(m_importer.isSupported(kNv12, kLinear));
    EXPECT_TRUE(m_importer.isSupported(kNv12, kLinear));
}

TEST_F(GlFrameImporterSupportTest, MissingGlExternalImageExtensionFails)
{
    displayHasExtensions("EGL_EXT_image_dma_buf_import EGL_EXT_image_dma_buf_import_modifiers", "GL_OES_texture_npot");
    displayListsModifiers({kLinear});
    EXPECT_FALSE(m_importer.isSupported(kNv12, kLinear));
}

TEST_F(GlFrameImporterSupportTest, ExtensionNameMustMatchWholeToken)
{
    displayHasExtensions("EGL_EXT_image_dma_buf_import_modifiers"); // contains the name only as a prefix
    displayListsModifiers({kLinear});
    EXPECT_FALSE(m_importer.isSupported(kNv12, kLinear));
}

/**
 * Through the capture: imports fail, the error is logged once, and every received descriptor is closed.
 */
TEST_F(GlFrameImporterSupportTest, MissingDmaBufImportFailsAcquisitionsAndClosesDescriptors)
{
    displayHasExtensions("EGL_EXT_image_dma_buf_import_modifiers");
    displayListsModifiers({kLinear});
    auto ipc = std::make_shared<NiceMock<VideoFrameCaptureIpcMock>>();
    auto importer = std::make_shared<GlFrameImporter>(m_gl, m_display, m_context);
    CaptureCore core{ipc, 5, importer};

    std::vector<int> fds;
    EXPECT_CALL(*ipc, acquireCurrentFrame(5))
        .Times(2)
        .WillRepeatedly(Invoke(
            [&fds](std::uint32_t)
            {
                CaptureAcquireResult result;
                result.status = CaptureAcquireStatus::OK;
                result.description.slotIndex = static_cast<std::uint32_t>(fds.size());
                result.description.acquisitionToken = fds.size() + 1;
                result.description.drmFormat = kNv12;
                result.description.objects = {{101 + fds.size(), 4096}};
                result.description.planes = {{0, 101 + fds.size(), 0, 2048, 1920}};
                const int kFd{memfd_create("gl-support-test", MFD_CLOEXEC)};
                fds.push_back(kFd);
                result.fds = {{101 + fds.size() - 1, kFd}};
                return result;
            }));
    ON_CALL(*ipc, releaseFrame(_, _, _, _)).WillByDefault(Return(true));

    ClientErrorLog log;
    EXPECT_EQ(core.acquireCurrentFrame().frame, nullptr);
    EXPECT_EQ(core.acquireCurrentFrame().frame, nullptr);
    EXPECT_EQ(log.count("EGL_EXT_image_dma_buf_import,"), 1U);
    for (int fd : fds)
        EXPECT_EQ(fcntl(fd, F_GETFD), -1); // closed
}
