#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Sky UK
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

set( CMAKE_INCLUDE_CURRENT_DIR ON )
set( CMAKE_CXX_STANDARD 14 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )

# Define a static library for Perfetto.
include_directories(../../../perfetto/sdk)
add_library(perfetto STATIC ../../../perfetto/sdk/perfetto.cc)

#include(ExternalProject)
#ExternalProject_Add(libnova
#    GIT_REPOSITORY    git clone https://android.googlesource.com/platform/external/perfetto
#    GIT_TAG           v29.0
#    UPDATE_COMMAND    ""
#    GIT_SUBMODULES    ""
#    BUILD_ALWAYS      TRUE
#    SOURCE_DIR       "${CMAKE_SOURCE_PATH}/external/perfetto"
#    CMAKE_ARGS       -DCMAKE_FIND_DEBUG_MODE=ON
#                     -DPLATFORM=${PLATFORM}
#                     -DCMAKE_INCLUDE_PATH=${CMAKE_INCLUDE_PATH}
#                     -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
#)

add_library(
        RialtoPerfetto
        STATIC

        source/TracingCategories.cpp
        source/RialtoPerfetto.cpp
    )

target_include_directories(
        RialtoPerfetto

        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/rialto>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../perfetto/sdk>
        $<INSTALL_INTERFACE:include/rialto>


        PRIVATE
        include
        ../../../perfetto/sdk
        $<TARGET_PROPERTY:RialtoServerCommon,INTERFACE_INCLUDE_DIRECTORIES>
    )

target_link_libraries(
        RialtoPerfetto

        PRIVATE
        RialtoLogging
        perfetto
)

