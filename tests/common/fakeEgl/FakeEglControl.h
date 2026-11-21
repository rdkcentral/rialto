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

#ifndef FAKE_EGL_CONTROL_H_
#define FAKE_EGL_CONTROL_H_

/**
 * @file FakeEglControl.h
 *
 * Control functions of the fake libEGL used by tests. Tests resolve them with dlsym on the loaded library.
 */

extern "C"
{
    /** Resets every counter and setting. */
    void fakeEglReset();
    /** EGL images created and not yet destroyed. */
    int fakeEglLiveImages();
    /** GL textures generated and not yet deleted. */
    int fakeEglLiveTextures();
    /** EGL contexts created and not yet destroyed. */
    int fakeEglLiveContexts();
    /** Number of EGL and GL calls made on the calling thread since the last reset of that thread's counter. */
    int fakeEglThreadCallCount();
    /** Resets the calling thread's call counter. */
    void fakeEglResetThreadCallCount();
    /** Makes eglCreateImageKHR fail (true) or succeed (false). */
    void fakeEglFailImages(bool fail);
    /** Sets the EGL extension string. */
    void fakeEglSetExtensions(const char *extensions);
}

#endif // FAKE_EGL_CONTROL_H_
