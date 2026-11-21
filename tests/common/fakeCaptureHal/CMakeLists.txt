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

# Fake vendor capture HAL library for tests: exports the extern "C" createCaptureSession() factory, like a vendor
# lib_capture_hal.so, plus the control functions in FakeCaptureHalControl.h. Tests load it with dlopen.
add_library(
        fake_capture_hal
        SHARED

        FakeCaptureHal.cpp
        )

target_include_directories(
        fake_capture_hal

        PUBLIC
        .
        $<TARGET_PROPERTY:RialtoCaptureHal,INTERFACE_INCLUDE_DIRECTORIES>
        )

# The control header only, for tests that resolve the functions with dlsym.
add_library(
        FakeCaptureHalControl
        INTERFACE
        )

target_include_directories(
        FakeCaptureHalControl
        INTERFACE
        .
        )

# The same, without the createCaptureSession() factory.
add_library(
        fake_capture_hal_without_factory
        SHARED

        FakeCaptureHalWithoutFactory.cpp
        )
