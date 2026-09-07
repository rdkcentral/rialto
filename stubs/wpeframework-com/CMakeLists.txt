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
include( GNUInstallDirs )
include(ExternalProject)

if (NOT THUNDER_TAG)
  set(THUNDER_TAG "R4.4.0")
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/com)

ExternalProject_Add(
  thunder-repo-com
  GIT_REPOSITORY https://github.com/rdkcentral/Thunder.git
  GIT_TAG ${THUNDER_TAG}
  PREFIX "git"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/com.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/Administrator.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/Communicator.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/ConnectorType.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/IteratorType.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/IIteratorType.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/IStringIterator.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/IUnknown.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/IValueIterator.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/ICOM.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/Messages.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/Module.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-com/Source/com/Ids.h
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/com
)

add_library(
    ThunderCOM
    INTERFACE
)

add_library(ThunderCOM::ThunderCOM ALIAS ThunderCOM)
add_dependencies( ThunderCOM thunder-repo-com )

include(CMakePackageConfigHelpers)
set(Thunder_COM_INCLUDE_INSTALL_DIR "${PROJECT_SOURCE_DIR}/stubs/thunder-com/third-party/Source/" )
set(PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/stubs/thunder-com/thundercom-config.cmake")

configure_package_config_file(thundercom-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/thundercom-config.cmake
  INSTALL_DESTINATION ${CMAKE_MODULE_PATH}
  PATH_VARS Thunder_COM_INCLUDE_INSTALL_DIR)

target_include_directories(
    ThunderCOM

    INTERFACE
    third-party/Source
)
