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

# Run the protoc tool to generate the code
set( Protobuf_IMPORT_DIRS "${CMAKE_SYSROOT}/usr/include" "${CMAKE_CURRENT_LIST_DIR}/../../../../ipc/common/proto/" )
protobuf_generate_cpp( PROTO_SRCS PROTO_HEADERS ../../../../proto/controlmodule.proto)

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

add_library (
        RialtoIpcServerStub
        STATIC

        ${PROTO_SRCS}
        ${PROTO_HEADERS}

        ServerStub.cpp
)

set_property (
        TARGET
        RialtoIpcServerStub
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_include_directories (
        RialtoIpcServerStub

        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}

        PRIVATE
        ../mocks
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcServer,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcCommon,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries (
        RialtoIpcServerStub

        PRIVATE
        RialtoIpcServer
        RialtoIpcCommon
        protobuf::libprotobuf
        GoogleTest::gtest
        GoogleTest::gmock
)