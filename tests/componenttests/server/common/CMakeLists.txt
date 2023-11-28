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
protobuf_generate_cpp( PROTO_SRCS PROTO_HEADERS ../../../../proto/servermanagermodule.proto)

list( GET PROTO_HEADERS 0 PROTO_HEADER )
get_filename_component( PROTO_DIR ${PROTO_HEADER} DIRECTORY )

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

add_library (
        RialtoServerComponentTestsCommon
        STATIC

        ${PROTO_SRCS}
        ${PROTO_HEADERS}

        MessageBuilders.cpp
)

set_property (
        TARGET
        RialtoServerComponentTestsCommon
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_include_directories (
        RialtoServerComponentTestsCommon

        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${PROTO_DIR}

        PRIVATE
)

target_link_libraries (
        RialtoServerComponentTestsCommon

        PRIVATE
        protobuf::libprotobuf
        GoogleTest::gtest
        GoogleTest::gmock
)

