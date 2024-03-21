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

set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )
include( GNUInstallDirs )
include(ExternalProject)

if (NOT THUNDER_CLIENT_TAG)
  set(THUNDER_CLIENT_TAG "d3e3d6c2be6516dd6b4ccfc04b129f8725b59baa")
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party/opencdm)

ExternalProject_Add(
  ThunderClientLibraries
  GIT_REPOSITORY https://github.com/rdkcentral/ThunderClientLibraries.git
  GIT_TAG ${THUNDER_CLIENT_TAG}
  PREFIX "git"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy 
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/ThunderClientLibraries/Source/ocdm/open_cdm.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/ThunderClientLibraries/Source/ocdm/open_cdm_ext.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/ThunderClientLibraries/Source/ocdm/adapter/open_cdm_adapter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/opencdm
)

find_package( PkgConfig REQUIRED )
pkg_check_modules( GStreamerApp REQUIRED IMPORTED_TARGET gstreamer-1.0 )

add_library(
  ocdm

  SHARED

  open_cdm.cpp
  open_cdm_ext.cpp
  open_cdm_adapter.cpp
  RialtoGStreamerEMEProtectionMetadata.cpp

        )
add_dependencies( ocdm ThunderClientLibraries )
add_library(ocdm::ocdm ALIAS ocdm)

include(CMakePackageConfigHelpers)
set(OCDM_INCLUDE_INSTALL_DIR "${PROJECT_SOURCE_DIR}/stubs/opencdm/third-party" )
set(PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/stubs/opencdm/ocdm-config.cmake")

configure_package_config_file(ocdm-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ocdm-config.cmake
  INSTALL_DESTINATION ${CMAKE_MODULE_PATH}
  PATH_VARS OCDM_INCLUDE_INSTALL_DIR)

target_include_directories(
      ocdm

      PRIVATE
      third-party
      ${GStreamerApp_INCLUDE_DIRS}
)

set_target_properties(
        ocdm
        PROPERTIES SOVERSION  ${PROJECT_VERSION_MAJOR}
                   VERSION    ${CMAKE_PROJECT_VERSION}
        )

install(FILES "${PROJECT_CONFIG_FILE}"
  DESTINATION ${CMAKE_MODULE_PATH}
  COMPONENT development
)

set (OCDM_PUBLIC_HEADER
      third-party/opencdm/open_cdm_adapter.h
      third-party/opencdm/open_cdm_ext.h
      third-party/opencdm/open_cdm.h
)

install (
          FILES ${OCDM_PUBLIC_HEADER}
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opencdm
)

install (
          FILES RialtoGStreamerEMEProtectionMetadata.h
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rialto
)

install (
        TARGETS ocdm LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
