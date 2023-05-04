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
include( FindProtobuf )
set( Protobuf_IMPORT_DIRS "${CMAKE_SYSROOT}/usr/include" )
protobuf_generate_cpp( PROTO_SRCS PROTO_HEADERS proto/rialtoipc.proto proto/rialtoipc-transport.proto )

# Get the directory where header was generated
list( GET PROTO_HEADERS 0 FIRST_PROTO_HEADER )
get_filename_component( PROTO_HEADER_DIR ${FIRST_PROTO_HEADER} DIRECTORY )

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )


add_library(
        RialtoIpcCommon
        STATIC

        ${PROTO_SRCS}
        ${PROTO_HEADERS}

        source/FileDescriptor.cpp
        source/SimpleBufferPool.cpp

        )

target_include_directories(
        RialtoIpcCommon

        PUBLIC
        include
        ${PROTO_HEADER_DIR}
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>

        PRIVATE
        source

        )


set_property(
        TARGET RialtoIpcCommon
        PROPERTY POSITION_INDEPENDENT_CODE ON

        )

target_link_libraries(
        RialtoIpcCommon

        PRIVATE
        protobuf::libprotobuf
        RialtoLogging
        )
