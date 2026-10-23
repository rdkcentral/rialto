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

set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )

option(RIALTO_ENABLE_AVMONITOR_CLIENT "Enable AVMonitor client integration in RialtoCommon profiler" OFF)

add_subdirectory(public)

add_library(
        RialtoCommon
        STATIC

        source/EventThread.cpp
        source/LinuxUtils.cpp
        source/Timer.cpp
        source/Profiler.cpp
    )

set_property (
        TARGET
        RialtoCommon
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_include_directories(
        RialtoCommon

        PUBLIC
        interface
        $<TARGET_PROPERTY:RialtoCommonPublic,INTERFACE_INCLUDE_DIRECTORIES>

        PRIVATE
        include
    )

target_link_libraries (
        RialtoCommon

        PRIVATE
        RialtoLogging
    )

if (RIALTO_ENABLE_AVMONITOR_CLIENT)
    set(AVMONITOR_CLIENT_TARGET "")

    if (TARGET WPEFrameworkAVMonitor)
        set(AVMONITOR_CLIENT_TARGET WPEFrameworkAVMonitor)
    elseif (TARGET WPEFramework::WPEFrameworkAVMonitor)
        set(AVMONITOR_CLIENT_TARGET WPEFramework::WPEFrameworkAVMonitor)
    else()
        find_package(PkgConfig QUIET)
        if (PkgConfig_FOUND)
            pkg_check_modules(AVMONITOR_CLIENT QUIET IMPORTED_TARGET WPEFrameworkAVMonitor)
            if (AVMONITOR_CLIENT_FOUND)
                set(AVMONITOR_CLIENT_TARGET PkgConfig::AVMONITOR_CLIENT)
            endif()
        endif()
    endif()

    if (AVMONITOR_CLIENT_TARGET STREQUAL "")
        message(FATAL_ERROR "RIALTO_ENABLE_AVMONITOR_CLIENT is ON, but WPEFrameworkAVMonitor was not found")
    endif()

    target_compile_definitions(RialtoCommon PRIVATE RIALTO_ENABLE_AVMONITOR_CLIENT)
    target_link_libraries(RialtoCommon PRIVATE ${AVMONITOR_CLIENT_TARGET})
endif()
