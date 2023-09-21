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

if( RIALTO_ENABLE_CONFIG_FILE )
    find_package( jsoncpp REQUIRED )
    set(RIALTO_CONFIG_SOURCE_FILES
        source/ConfigReader.cpp
        source/JsonCppWrapper.cpp
        source/FileReaderFactory.cpp
        source/JsonCppWrapperFactory.cpp
        source/ConfigReaderFactory.cpp)
endif()

find_package( PkgConfig REQUIRED )

add_library (
        RialtoServerManager SHARED
        source/ServerManagerServiceFactory.cpp
        source/ServerManagerService.cpp
        source/ServiceContext.cpp
        ${RIALTO_CONFIG_SOURCE_FILES}
)

set_target_properties (
        RialtoServerManager
        PROPERTIES LINK_FLAGS "-Wl,--unresolved-symbols=report-all"
                   SOVERSION  ${PROJECT_VERSION_MAJOR}
                   VERSION    ${CMAKE_PROJECT_VERSION}
)

target_include_directories (
        RialtoServerManager
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/rialto/servermanager>
        PRIVATE
        $<TARGET_PROPERTY:RialtoServerManagerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerManagerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries (
        RialtoServerManager
        PRIVATE
        RialtoLogging
        RialtoServerManagerCommon
        RialtoServerManagerIpc
        Threads::Threads
)

if( RIALTO_ENABLE_CONFIG_FILE )
    add_compile_definitions(RIALTO_ENABLE_CONFIG_FILE)
    add_compile_definitions(RIALTO_CONFIG_PATH="${RIALTO_CONFIG_FILE_DIR}/rialto-config.json")

    target_link_libraries(
        RialtoServerManager
        PRIVATE
        JsonCpp::JsonCpp
        )
endif()

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoServerManager
        PRIVATE
        gcov
        )
endif()

if( NOT CMAKE_BUILD_TYPE STREQUAL "UnitTests" )
        include( GNUInstallDirs )
        install (
                TARGETS RialtoServerManager LIBRARY
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
endif()
