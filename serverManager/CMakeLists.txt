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

find_package( mongoose REQUIRED )

add_subdirectory(common)
add_subdirectory(ipc)
add_subdirectory(service)
add_subdirectory(public)

add_executable (
        RialtoServerManagerSim

        serverManagerSim/commands/CommandFactory.cpp
        serverManagerSim/commands/GetAppInfo.cpp
        serverManagerSim/commands/GetState.cpp
        serverManagerSim/commands/Quit.cpp
        serverManagerSim/commands/SetLog.cpp
        serverManagerSim/commands/SetState.cpp
        serverManagerSim/commands/Unknown.cpp

        serverManagerSim/HttpRequest.cpp
        serverManagerSim/RialtoServerManagerSim.cpp
        serverManagerSim/StateObserver.cpp
        serverManagerSim/TestService.cpp
)

target_include_directories (
        RialtoServerManagerSim

        PUBLIC
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerManagerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        mongoose
)

if( RIALTO_ENABLE_CONFIG_FILE )
    if (NOT RIALTO_CONFIG_FILE_DIR)
        set(RIALTO_CONFIG_FILE_DIR /etc/)
    endif()

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

    if (NOT NUM_OF_PRELOADED_SERVERS)
        set( NUM_OF_PRELOADED_SERVERS 0 )
    endif()

    if (NOT LOG_LEVEL)
        set( LOG_LEVEL 3 )
    endif()

    configure_file(
        config/rialto-config.in.json
        rialto-config.json
        @ONLY )

    install(
        FILES       ${CMAKE_CURRENT_BINARY_DIR}/rialto-config.json
        DESTINATION ${RIALTO_CONFIG_FILE_DIR} )
endif()

target_link_libraries (
        RialtoServerManagerSim

        RialtoServerManager
        RialtoLogging
        Threads::Threads
        mongoose
        dl
)

install (
        TARGETS RialtoServerManagerSim
        DESTINATION bin
)

