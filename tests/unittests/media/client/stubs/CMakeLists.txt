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

set( Protobuf_IMPORT_DIRS "${CMAKE_SYSROOT}/usr/include" "${CMAKE_CURRENT_LIST_DIR}/../../../../../ipc/common/proto" )
protobuf_generate_cpp( PROTO_SRCS PROTO_HEADERS ../../../../../proto/rialtocommon.proto ../../../../../proto/mediapipelinemodule.proto  ../../../../../proto/mediapipelinecapabilitiesmodule.proto
        ../../../../../proto/mediakeysmodule.proto ../../../../../proto/mediakeyscapabilitiesmodule.proto ../../../../../proto/controlmodule.proto ../../../../../proto/webaudioplayermodule.proto )

list( GET PROTO_HEADERS 0 MEDIA_PIPELINE_PROTO_HEADER )
list( GET PROTO_HEADERS 1 RIALTO_CONTROL_PROTO_HEADER )
get_filename_component( MEDIA_PIPELINE_PROTO_DIR ${MEDIA_PIPELINE_PROTO_HEADER} DIRECTORY )
get_filename_component( RIALTO_CONTROL_PROTO_DIR ${RIALTO_CONTROL_PROTO_HEADER} DIRECTORY )


# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )

add_library (
        RialtoClientStub
        STATIC

        ${PROTO_SRCS}
        ${PROTO_HEADERS}

        MediaPipelineIpcFactoryStub.cpp
)

set_property (
        TARGET
        RialtoClientStub
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_include_directories (
        RialtoClientStub

        PRIVATE
        ${MEDIA_PIPELINE_PROTO_DIR}
        ${RIALTO_CONTROL_PROTO_DIR}
        ../mocks
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientIpcImpl,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientMocks,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries (
        RialtoClientStub

        PRIVATE
        GoogleTest::gtest
        GoogleTest::gmock
)
