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

# Run the protoc tool to generate the code

set( Protobuf_IMPORT_DIRS "${CMAKE_SYSROOT}/usr/include" "${CMAKE_CURRENT_LIST_DIR}/../../ipc/common/proto/" )
protobuf_generate_cpp( PROTO_SRCS PROTO_HEADERS proto/testmodule.proto proto/testmodule.proto )

list( GET PROTO_HEADERS 0 PROTO_HEADER )
get_filename_component( PROTO_DIR ${PROTO_HEADER} DIRECTORY )

add_gtests (
        RialtoIpcUnitTests

        # proto files
        ${PROTO_SRCS}
        ${PROTO_HEADERS}

        IpcTest.cpp
        )

add_subdirectory(mocks)
add_subdirectory(stubs)

target_include_directories(
        RialtoIpcUnitTests

        PRIVATE
        ${PROTO_DIR}
        ${PROTO_DIR}
        $<TARGET_PROPERTY:RialtoIpcClient,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcServer,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcStub,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcMocks,INTERFACE_INCLUDE_DIRECTORIES>
        )

target_link_libraries(
        RialtoIpcUnitTests

        # # Link application source
        RialtoIpcClient
        RialtoIpcCommon
        RialtoIpcServer
        RialtoIpcStub
        )

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoIpcUnitTests

        gcov
        )
endif()


