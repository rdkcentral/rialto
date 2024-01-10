#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 Sky UK
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

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

add_library (
        RialtoServerComponentTestsFixtures
        STATIC
        RialtoServerComponentTest.cpp
        MediaPipelineTest.cpp
)

set_property (
        TARGET
        RialtoServerComponentTestsFixtures
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_include_directories (
        RialtoServerComponentTestsFixtures

        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        $<TARGET_PROPERTY:RialtoServerComponentTestsCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerComponentTestsMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerComponentTestsStubs,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerService,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoWrappers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerGstPlayer,INCLUDE_DIRECTORIES> # For GstRialtoSrc
        ${GStreamerApp_INCLUDE_DIRS}

        PRIVATE
)

target_link_libraries (
        RialtoServerComponentTestsFixtures

        PRIVATE
        RialtoServerComponentTestsCommon
        RialtoServerComponentTestsStubs
        RialtoServerService
        RialtoPlayerCommon
        GoogleTest::gtest
        GoogleTest::gmock
)
