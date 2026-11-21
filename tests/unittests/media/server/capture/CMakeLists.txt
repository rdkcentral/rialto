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

add_gtests (
        RialtoServerCaptureUnitTests

        CaptureHalLibraryTests.cpp
        CaptureSessionGuardTests.cpp
        CapturedFrameTraitsTests.cpp
        CapturedFrameUtilsTests.cpp
        )

target_include_directories(
        RialtoServerCaptureUnitTests

        PRIVATE
        $<TARGET_PROPERTY:ExternalLibraryMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoWrappers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:FakeCaptureHalControl,INTERFACE_INCLUDE_DIRECTORIES>
        ../../../../../wrappers/include
        )

target_link_libraries(
        RialtoServerCaptureUnitTests

        RialtoServerCapture
        RialtoWrappers
        RialtoLogging
        ${CMAKE_DL_LIBS}
        )

# the loader tests dlopen these at run time
add_dependencies( RialtoServerCaptureUnitTests fake_capture_hal fake_capture_hal_without_factory )
target_compile_definitions(
        RialtoServerCaptureUnitTests

        PRIVATE
        FAKE_CAPTURE_HAL_PATH="$<TARGET_FILE:fake_capture_hal>"
        FAKE_CAPTURE_HAL_WITHOUT_FACTORY_PATH="$<TARGET_FILE:fake_capture_hal_without_factory>"
        )
