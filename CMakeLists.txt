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

# Preprocesser Variable
add_compile_definitions(PROJECT_VER_MAJOR="${PROJECT_VERSION_MAJOR}")
add_compile_definitions(PROJECT_VER_MINOR="${PROJECT_VERSION_MINOR}")
add_compile_definitions(PROJECT_VER_PATCH="${PROJECT_VERSION_PATCH}")

set(CMAKE_CXX_STANDARD 17)

if ( COVERAGE_ENABLED )
    add_compile_options(-coverage)
endif()

add_compile_options(-Wall -Werror)

option(RIALTO_LOG_FATAL_ENABLED "Enable fatal logging for RialtoServer" ON)
option(RIALTO_LOG_ERROR_ENABLED "Enable error logging for RialtoServer" ON)
option(RIALTO_LOG_WARN_ENABLED "Enable warning logging for RialtoServer" ON)
option(RIALTO_LOG_MIL_ENABLED "Enable milestone logging for RialtoServer" ON)

if( RIALTO_BUILD_TYPE STREQUAL "Debug" )
    message("_______________DEBUG MODE_______________")
    option(RIALTO_LOG_INFO_ENABLED "Enable info logging for RialtoServer" ON)
    option(RIALTO_LOG_DEBUG_ENABLED "Enable debug logging for RialtoServer" ON)
else()
    message("_______________RELEASE MODE (DEFAULT)_______________")
    option(RIALTO_LOG_INFO_ENABLED "Enable info logging for RialtoServer" OFF)
    option(RIALTO_LOG_DEBUG_ENABLED "Enable debug logging for RialtoServer" OFF)
endif()

if (NOT RIALTO_LOG_FATAL_ENABLED OR 
    NOT RIALTO_LOG_ERROR_ENABLED OR
    NOT RIALTO_LOG_WARN_ENABLED OR
    NOT RIALTO_LOG_MIL_ENABLED OR
    NOT RIALTO_LOG_INFO_ENABLED OR
    NOT RIALTO_LOG_DEBUG_ENABLED )

    message("_______________COMPILATION ERRORS IGNORED_______________")
    add_compile_options(-Wno-error=unused-function -Wno-error=unused-variable)

endif()
   
if ( RIALTO_LOG_FATAL_ENABLED )
    message("RIALTO_LOG_FATAL IS ENABLED")
    add_compile_definitions( RIALTO_LOG_FATAL_ENABLED )
endif()

if ( RIALTO_LOG_ERROR_ENABLED )
    message("RIALTO_LOG_ERROR IS ENABLED")
    add_compile_definitions( RIALTO_LOG_ERROR_ENABLED )
endif()

if ( RIALTO_LOG_WARN_ENABLED )
    message("RIALTO_LOG_WARN IS ENABLED")
    add_compile_definitions( RIALTO_LOG_WARN_ENABLED )
endif()

if ( RIALTO_LOG_MIL_ENABLED )
    message("RIALTO_LOG_MIL IS ENABLED")
    add_compile_definitions( RIALTO_LOG_MIL_ENABLED )
endif()

if ( RIALTO_LOG_INFO_ENABLED )
    message("RIALTO_LOG_INFO IS ENABLED")
    add_compile_definitions( RIALTO_LOG_INFO_ENABLED )
endif()

if ( RIALTO_LOG_DEBUG_ENABLED )
    message("RIALTO_LOG_DEBUG IS ENABLED")
    add_compile_definitions( RIALTO_LOG_DEBUG_ENABLED )
endif()

if ( RIALTO_ENABLE_DECRYPT_BUFFER )
    add_compile_definitions( RIALTO_ENABLE_DECRYPT_BUFFER )
endif()

# Retrieve the commit ID
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} 
    RESULT_VARIABLE RESULT 
    OUTPUT_VARIABLE COMMIT_ID 
    OUTPUT_STRIP_TRAILING_WHITESPACE 
)

if(RESULT)
    message("Failed to get git commit ID: ${RESULT}")
endif()

# Preprocesser Variable
add_compile_definitions(COMMIT_ID="${COMMIT_ID}")

# Retrieve the branch 
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE RESULT_BRANCH
    OUTPUT_VARIABLE BUILD_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(RESULT_BRANCH)
    message("Failed to get git branch: ${RESULT_BRANCH}")
endif()

# Preprocessor Variable
add_compile_definitions(BUILD_BRANCH="${BUILD_BRANCH}")

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

# Options to disable building some of the components
option(ENABLE_SERVER "Enable building RialtoServer" ON)
option(ENABLE_SERVER_MANAGER "Enable building RialtoServerManagerSim" ON)

if ( NOT ENVIRONMENT_VARIABLES)
    set( ENVIRONMENT_VARIABLES "\"XDG_RUNTIME_DIR=/tmp\",\"WAYLAND_DISPLAY=westeros-rialto\",\"RIALTO_SINKS_RANK=0\",\"GST_REGISTRY=/tmp/rialto-server-gstreamer-cache.bin\",\"WESTEROS_SINK_USE_ESSRMGR=1\"" )
endif()

if (NOT SESSION_SERVER_PATH)
    set( SESSION_SERVER_PATH "\"/usr/bin/RialtoServer\"" )
endif()

if (NOT STARTUP_TIMEOUT_MS)
    set( STARTUP_TIMEOUT_MS 0 )
endif()

if (NOT HEALTHCHECK_INTERVAL_S)
    set( HEALTHCHECK_INTERVAL_S 5 )
endif()

if (NOT SOCKET_PERMISSIONS)
    set( SOCKET_PERMISSIONS 666 )
endif()
string(REGEX MATCH "^([0-9]+)([0-9]+)([0-9]+)" PERMISSIONS_MATCH ${SOCKET_PERMISSIONS})
set(SOCKET_PERMISSIONS_OWNER ${CMAKE_MATCH_1})
set(SOCKET_PERMISSIONS_GROUP ${CMAKE_MATCH_2})
set(SOCKET_PERMISSIONS_OTHER ${CMAKE_MATCH_3})

# Empty strings for "owner" and "group" means that chown() won't be called. This will leave the created
# socket being owned by the user executing the code (and the group would be their primary group)
if (NOT SOCKET_OWNER)
    set(SOCKET_OWNER "\"\"" )
endif()
if (NOT SOCKET_GROUP)
    set(SOCKET_GROUP "\"\"" )
endif()

if (NOT NUM_OF_PRELOADED_SERVERS)
    set( NUM_OF_PRELOADED_SERVERS 0 )
endif()

if (NOT LOG_LEVEL)
    set( LOG_LEVEL 3 )
endif()

if (NOT NUM_OF_PINGS_BEFORE_RECOVERY)
    set( NUM_OF_PINGS_BEFORE_RECOVERY 3 )
endif()

if( NATIVE_BUILD )
    add_subdirectory( stubs/rdk_gstreamer_utils )
    add_subdirectory( stubs/opencdm )

    include( GNUInstallDirs )
    configure_file( pkg-config/RialtoClient.in.pc ${CMAKE_BINARY_DIR}/RialtoClient.pc @ONLY )
    install (
          FILES ${CMAKE_BINARY_DIR}/RialtoClient.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
endif()

# Include the new IPC library components
add_subdirectory( ipc )

# Add the rialto targets
add_subdirectory( common )
add_subdirectory( media )
add_subdirectory( logging )

if( ENABLE_SERVER_MANAGER )
    add_subdirectory( serverManager )
endif()

# Config and target for building the unit tests
if( CMAKE_BUILD_FLAG STREQUAL "UnitTests" )

    include( cmake/googletest.cmake )

    add_subdirectory( tests/common EXCLUDE_FROM_ALL )
    add_subdirectory( tests/logging EXCLUDE_FROM_ALL )
    add_subdirectory( tests/serverManager EXCLUDE_FROM_ALL )
    add_subdirectory( tests/media/client EXCLUDE_FROM_ALL )
    add_subdirectory( tests/media/server EXCLUDE_FROM_ALL )
    add_subdirectory( tests/media/common EXCLUDE_FROM_ALL )
    add_subdirectory( tests/media/interface EXCLUDE_FROM_ALL )
    add_subdirectory( tests/ipc EXCLUDE_FROM_ALL )

endif()
