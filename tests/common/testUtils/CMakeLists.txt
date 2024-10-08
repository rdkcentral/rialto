#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 Sky UK
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

set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )

add_library(
        RialtoTestCommonUtils
        STATIC

        WebAudioTestCommon.cpp
        GstExpect.cpp
    )

target_include_directories(
        RialtoTestCommonUtils

        INTERFACE
        .

        PRIVATE
        $<TARGET_PROPERTY:RialtoWrappers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ExternalLibraryMocks,INTERFACE_INCLUDE_DIRECTORIES>
        ${GStreamerApp_INCLUDE_DIRS}
    )

target_link_libraries (
        RialtoTestCommonUtils

        PRIVATE
        GoogleTest::gtest
        GoogleTest::gmock
)
