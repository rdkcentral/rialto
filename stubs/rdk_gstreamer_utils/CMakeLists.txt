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

set( CMAKE_CXX_STANDARD 14 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )

include(ExternalProject)

if (NOT RDK_GSTREAMER_UTILS_TAG)
  set(RDK_GSTREAMER_UTILS_TAG "9f7bc92775d6b1c630952c2a53f807e838d38267")
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party)

ExternalProject_Add(
  rdk-gstreamer-utils-repo
  GIT_REPOSITORY https://code.rdkcentral.com/r/rdk/components/opensource/gstreamer-direct-platform
  GIT_TAG ${RDK_GSTREAMER_UTILS_TAG}
  PREFIX "git"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy 
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/rdk-gstreamer-utils-repo/rdk_gstreamer_utils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party
)

find_package( PkgConfig REQUIRED )
pkg_check_modules( GStreamerApp REQUIRED IMPORTED_TARGET gstreamer-1.0 )

add_library(
    rdkgstreamerutils

    SHARED
    rdk_gstreamer_utils.cpp
        )

target_include_directories(
    rdkgstreamerutils

    PUBLIC
    third-party

    PRIVATE
    ${GStreamerApp_INCLUDE_DIRS}
    )

add_dependencies( rdkgstreamerutils rdk-gstreamer-utils-repo )
