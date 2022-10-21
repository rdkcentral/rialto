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

set( CMAKE_CXX_STANDARD 14 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )

add_subdirectory(common)
add_subdirectory(gstplayer)
add_subdirectory(main)
add_subdirectory(ipc)
add_subdirectory(service)
add_subdirectory(ocdm)

add_executable (
        RialtoServer

        service/source/main.cpp
)


target_include_directories (
        RialtoServer

        PRIVATE
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerGstPlayer,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerIpc,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMain,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerService,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
)

set_target_properties(
        RialtoServer
        PROPERTIES LINK_FLAGS "-Wl,--unresolved-symbols=report-all"
)

if( CMAKE_BUILD_FLAG STREQUAL "UnitTests" )
        set( RialtoServerStubsLibs RialtoServerStubs )
endif()

target_link_libraries(
        RialtoServer

        PRIVATE
        RialtoServerIpc
        RialtoServerMain
        RialtoServerService
        ${RialtoServerStubsLibs}
)

install(
        TARGETS RialtoServer
        DESTINATION bin
)
