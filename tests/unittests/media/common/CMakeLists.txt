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

set( Protobuf_IMPORT_DIRS "${CMAKE_SYSROOT}/usr/include" "${CMAKE_CURRENT_LIST_DIR}/../../../ipc/common/proto/" )
protobuf_generate_cpp( PROTO_SRCS PROTO_HEADERS ../../../proto/metadata.proto )

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

add_gtests (
        RialtoPlayerCommonUnitTests

        ${PROTO_SRCS}
        ${PROTO_HEADERS}

        # tests
        mediaFrameWriterV1/CreateTest.cpp
        mediaFrameWriterV1/WriteFrameTest.cpp

        mediaFrameWriterV2/CreateTest.cpp
        mediaFrameWriterV2/WriteFrameTest.cpp

        schemaVersion/SchemaVersionTest.cpp
        )

add_subdirectory(mocks)

target_include_directories(
        RialtoPlayerCommonUnitTests

        PRIVATE
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerCommon,INCLUDE_DIRECTORIES>

        mocks
        )

target_link_libraries(
        RialtoPlayerCommonUnitTests

        # # Link application source
        RialtoPlayerCommon
        protobuf::libprotobuf
        )

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoPlayerCommonUnitTests

        gcov
        )
endif()

