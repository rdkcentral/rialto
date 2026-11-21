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
#include "FakeEglControl.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <dlfcn.h>

using firebolt::rialto::client::EglLibrary;
using namespace firebolt::rialto::client::egl;

class EglLibraryTest : public ::testing::Test
{
protected:
    void TearDown() override { unsetenv(EglLibrary::kLibraryEnv); }
};

TEST_F(EglLibraryTest, ShouldResolveEveryEntryPointFromTheLoadedLibrary)
{
    setenv(EglLibrary::kLibraryEnv, FAKE_EGL_PATH, 1);
    EglLibrary library;
    auto functions = library.functions();
    ASSERT_NE(functions, nullptr);
    EXPECT_EQ(library.functions(), functions); // resolved once

    // calls reach the fake library
    GLuint texture{0};
    functions->glGenTextures(1, &texture);
    EXPECT_NE(texture, 0U);
    functions->glDeleteTextures(1, &texture);
    EXPECT_STREQ(reinterpret_cast<const char *>(functions->glGetString(0x1F03)), "GL_OES_EGL_image_external");
    EXPECT_EQ(functions->eglGetCurrentContext(), nullptr);
}

TEST_F(EglLibraryTest, ShouldGiveNoFunctionsWhenEglCannotBeLoaded)
{
    setenv(EglLibrary::kLibraryEnv, "/nonexistent/libEGL.so.1", 1);
    EglLibrary library;
    EXPECT_FALSE(library.load());
    EXPECT_EQ(library.functions(), nullptr);
}

TEST_F(EglLibraryTest, ShouldGiveNoFunctionsWhenEntryPointsAreMissing)
{
    // a library that exports no EGL entry points at all
    setenv(EglLibrary::kLibraryEnv, FAKE_NOT_EGL_PATH, 1);
    EglLibrary library;
    EXPECT_TRUE(library.load());
    EXPECT_EQ(library.functions(), nullptr);
}
