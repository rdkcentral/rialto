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

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)

set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_POSITION_INDEPENDENT_CODE ON )
include( CheckCCompilerFlag )
include( CheckCXXCompilerFlag )

set(LIB_RIALTO_LOGGING_SOURCES
        source/EnvVariableParser.cpp
        source/RialtoLogging.cpp
        )

set(LIB_RIALTO_LOGGING_PUBLIC_HEADERS
        include/RialtoLogging.h
        )

add_library(RialtoLogging STATIC ${LIB_RIALTO_LOGGING_SOURCES})

target_include_directories(
        RialtoLogging

        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/rialto>
        )
