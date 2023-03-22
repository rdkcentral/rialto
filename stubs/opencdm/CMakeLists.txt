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

if (NOT THUNDER_CLIENT_TAG)
  set(THUNDER_CLIENT_TAG "d3e3d6c2be6516dd6b4ccfc04b129f8725b59baa")
endif()

ExternalProject_Add(
  ThunderClientLibraries
  GIT_REPOSITORY https://github.com/rdkcentral/ThunderClientLibraries.git
  GIT_TAG ${THUNDER_CLIENT_TAG}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy 
  ${CMAKE_CURRENT_SOURCE_DIR}/ThunderClientLibraries-prefix/src/ThunderClientLibraries/Source/ocdm/open_cdm.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/ThunderClientLibraries-prefix/src/ThunderClientLibraries/Source/ocdm/open_cdm_ext.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/ThunderClientLibraries-prefix/src/ThunderClientLibraries/Source/ocdm/adapter/open_cdm_adapter.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/
)

add_library(
  ocdm

  SHARED

  open_cdm.cpp
  open_cdm_ext.cpp
  open_cdm_adapter.cpp

        )
add_dependencies( ocdm ThunderClientLibraries )
add_library(ocdm::ocdm ALIAS ocdm)

include(CMakePackageConfigHelpers)
set(OCDM_INCLUDE_INSTALL_DIR "${PROJECT_BINARY_DIR}/stubs" )
set(PROJECT_CONFIG_FILE         "${PROJECT_BINARY_DIR}/ocdm-config.cmake")

configure_package_config_file(ocdm-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ocdm-config.cmake
  INSTALL_DESTINATION ${CMAKE_MODULE_PATH}
  PATH_VARS OCDM_INCLUDE_INSTALL_DIR)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ocdm-config.cmake"
  DESTINATION ${CMAKE_MODULE_PATH}
  COMPONENT development
)

install(
  TARGETS ocdm
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)
