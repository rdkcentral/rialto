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

# RIALTO-197: deprecated-declarations error in the latest stable2 for gstreamer.
# Should be removed once the issue is fixed.
add_compile_options(
  "-Wno-deprecated-declarations"
)

find_package( PkgConfig REQUIRED )
pkg_check_modules( GStreamerApp REQUIRED IMPORTED_TARGET gstreamer-app-1.0 gstreamer-pbutils-1.0 gstreamer-audio-1.0)

if( NATIVE_BUILD )
    set(ocdm_DIR "${CMAKE_BINARY_DIR}/stubs/opencdm/" )
endif()

if( RIALTO_ENABLE_CONFIG_FILE )
    find_package( jsoncpp REQUIRED )
    add_compile_definitions(JSONCPP_ENABLED)
    set(JSONCPP_WRAPPER_SOURCE_FILES
        source/JsonCppWrapper.cpp
        source/JsonCppWrapperFactory.cpp)
endif()

if( NOT CMAKE_BUILD_FLAG STREQUAL "UnitTests" AND NOT CMAKE_BUILD_FLAG STREQUAL "ComponentTests" )
    add_compile_definitions(WRAPPERS_ENABLED)

    find_package( ocdm REQUIRED )

    set(RDK_INCLUDES
        ${OCdm_INCLUDE_DIRS})

    set(RDK_LIBS
        rdkgstreamerutils
        ocdm::ocdm
        ${CMAKE_DL_LIBS})

    set(RDK_SOURCES
        source/GlibWrapper.cpp
        source/Ocdm.cpp
        source/OcdmCommon.cpp
        source/OcdmSystem.cpp
        source/OcdmSession.cpp
        source/RdkGstreamerUtilsWrapper.cpp)
endif()

add_library(
        RialtoWrappers

        STATIC
        source/FactoryAccessor.cpp
        source/GlibWrapperFactoryAccessor.cpp
        source/GstWrapper.cpp
        source/LinuxWrapper.cpp
        ${RDK_SOURCES}
        ${JSONCPP_WRAPPER_SOURCE_FILES}
    )

set_property (
        TARGET
        RialtoWrappers
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_include_directories(
        RialtoWrappers

        PUBLIC
        interface

        PRIVATE
        include
        ${GStreamerApp_INCLUDE_DIRS}
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        ${RDK_INCLUDES}
    )

target_link_libraries (
        RialtoWrappers

        PRIVATE
        ${RDK_LIBS}
        ${GStreamerApp_LIBRARIES}
    )

if( RIALTO_ENABLE_CONFIG_FILE )
    target_link_libraries(
        RialtoWrappers
        PRIVATE
        JsonCpp::JsonCpp
        )
endif()

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoWrappers
        PRIVATE
        gcov
        )
endif()
