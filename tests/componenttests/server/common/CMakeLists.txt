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
        RialtoServerComponentTestsCommon
        STATIC

        EventRanger.cpp
        MessageBuilders.cpp
        SegmentBuilder.cpp
        ShmHandle.cpp
)

set_property (
        TARGET
        RialtoServerComponentTestsCommon
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_include_directories (
        RialtoServerComponentTestsCommon

        PUBLIC
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcClient,INTERFACE_INCLUDE_DIRECTORIES>
        ${CMAKE_CURRENT_LIST_DIR}

        PRIVATE
)

target_link_libraries (
        RialtoServerComponentTestsCommon

        PRIVATE
        RialtoProtobuf
        GoogleTest::gtest
        GoogleTest::gmock
)

