#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2026 Sky UK
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

include( GNUInstallDirs )

# The SoC capture HAL interface. Header only: vendors build their capture library against it,
# and the server loads that library at run time.
add_library(
        RialtoCaptureHal
        INTERFACE
        )

target_include_directories(
        RialtoCaptureHal
        INTERFACE
        include
        )

# Rialto's server-side capture code, built on the HAL interface.
add_library(
        RialtoServerCapture
        STATIC

        source/CaptureRecordValidator.cpp
        source/CaptureSessionGuard.cpp
        source/CapturedFrameUtils.cpp
        )

target_include_directories(
        RialtoServerCapture
        PUBLIC
        include
        )

set_property(
        TARGET RialtoServerCapture
        PROPERTY POSITION_INDEPENDENT_CODE ON
        )

target_link_libraries(
        RialtoServerCapture
        PUBLIC
        RialtoCaptureHal

        PRIVATE
        RialtoLogging
        )

install (
        FILES include/ICaptureSession.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rialto/capture
        )

configure_file( ${CMAKE_SOURCE_DIR}/pkg-config/RialtoCaptureHal.in.pc ${CMAKE_BINARY_DIR}/RialtoCaptureHal.pc @ONLY )
install (
        FILES ${CMAKE_BINARY_DIR}/RialtoCaptureHal.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        )
