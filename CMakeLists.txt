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

cmake_minimum_required( VERSION 3.10 )
project( Rialto LANGUAGES C CXX VERSION 1.0.0 )

if ( COVERAGE_ENABLED )
    add_compile_options(-coverage)
endif()

add_compile_options(-Wall -Werror)

# Add our local cmake directory to search for components
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

# Thread is just the generic lib link for the pthread libraries (on platforms
# that don't have a separate pthread library this is a NOP)
find_package( Threads REQUIRED )

# Allow setting the path to the protoc compiler binary to use on the cmake
# command line - likely to need this for SDK builds as SDK doesn't
# contain the tools by default
set( PROTOC_PATH "" CACHE STRING "Optional path to the host protobuf compiler (protoc) tool" )
if( PROTOC_PATH )
    add_executable( protobuf::protoc IMPORTED )
    set_target_properties( protobuf::protoc PROPERTIES IMPORTED_LOCATION "${PROTOC_PATH}" )
endif()

# Import protobuf package
find_package( Protobuf REQUIRED )

# Include the new IPC library components
add_subdirectory( ipc )

# Add the rialto targets
add_subdirectory( common )
add_subdirectory( media )
add_subdirectory( logging )
add_subdirectory( serverManager )


# Config and target for building the unit tests
if( CMAKE_BUILD_FLAG STREQUAL "UnitTests" )

    include( cmake/googletest.cmake )

    add_subdirectory( tests/common EXCLUDE_FROM_ALL )
    add_subdirectory( tests/logging EXCLUDE_FROM_ALL )
    add_subdirectory( tests/serverManager EXCLUDE_FROM_ALL )
    add_subdirectory( tests/media/client EXCLUDE_FROM_ALL )
    add_subdirectory( tests/media/server EXCLUDE_FROM_ALL )
    add_subdirectory( tests/media/common EXCLUDE_FROM_ALL )
    add_subdirectory( tests/media/public EXCLUDE_FROM_ALL )
    add_subdirectory( tests/ipc EXCLUDE_FROM_ALL )

endif()
