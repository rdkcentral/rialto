#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2026 Sky UK
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Fake libEGL for tests: exports the EGL and GL entry points GL video frame capture resolves, plus the control
# functions in FakeEglControl.h. Tests load it by setting RIALTO_EGL_LIBRARY to its path.
add_library(
        fake_egl
        SHARED

        FakeEgl.cpp
        )

# The control header only, for tests that resolve the functions with dlsym.
add_library(
        FakeEglControl
        INTERFACE
        )

target_include_directories(
        FakeEglControl
        INTERFACE
        .
        )
