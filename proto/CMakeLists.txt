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
message("${CMAKE_CURRENT_LIST_DIR}")
set( Protobuf_IMPORT_DIRS "${CMAKE_SYSROOT}/usr/include" "${CMAKE_CURRENT_LIST_DIR}/../ipc/common/proto" )
protobuf_generate_cpp( PROTO_SRCS PROTO_HEADERS rialtocommon.proto mediapipelinemodule.proto  mediapipelinecapabilitiesmodule.proto
        mediakeysmodule.proto mediakeyscapabilitiesmodule.proto controlmodule.proto webaudioplayermodule.proto rialtoipc.proto 
        metadata.proto servermanagermodule.proto)

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

function(get_unique_paths result_list)
    set(unique_paths "")
    foreach(path ${ARGN})
        get_filename_component(dir ${path} DIRECTORY)
        list(APPEND unique_paths ${dir})
    endforeach()
    list(REMOVE_DUPLICATES unique_paths)
    set(${result_list} ${unique_paths} PARENT_SCOPE)
endfunction()
get_unique_paths(UNIQUE_INCLUDE_PATHS ${PROTO_HEADERS})
message("${UNIQUE_INCLUDE_PATHS}")

add_library (
        RialtoProtobuf
        SHARED

        ${PROTO_SRCS}
        ${PROTO_HEADERS}
)

target_include_directories( 
        RialtoProtobuf

        PUBLIC
        ${UNIQUE_INCLUDE_PATHS}
)

set_property (
        TARGET
        RialtoProtobuf
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_link_libraries (
        RialtoProtobuf

        PRIVATE
        protobuf::libprotobuf
)

install( TARGETS RialtoProtobuf LIBRARY
         DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
