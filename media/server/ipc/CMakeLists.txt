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
set( Protobuf_IMPORT_DIRS "${CMAKE_SYSROOT}/usr/include" "${CMAKE_CURRENT_LIST_DIR}/../../../ipc/common/proto" )
protobuf_generate_cpp( PROTO_SRCS PROTO_HEADERS proto/rialtocommon.proto proto/mediapipelinemodule.proto proto/mediapipelinecapabilitiesmodule.proto
        proto/mediakeysmodule.proto proto/mediakeyscapabilitiesmodule.proto proto/servermanagermodule.proto proto/rialtocontrolmodule.proto  proto/webaudioplayermodule.proto )

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

add_library (
        RialtoServerIpc
        STATIC

        ${PROTO_SRCS}
        ${PROTO_HEADERS}

        source/ApplicationManagementServer.cpp
        source/IpcFactory.cpp
        source/MediaPipelineClient.cpp
        source/MediaPipelineModuleService.cpp
        source/MediaPipelineCapabilitiesModuleService.cpp
        source/MediaKeysClient.cpp
        source/MediaKeysModuleService.cpp
        source/MediaKeysCapabilitiesModuleService.cpp
        source/RialtoControlModuleService.cpp
        source/ServerManagerModuleService.cpp
        source/SessionManagementServer.cpp
        source/SetLogLevelsService.cpp
        source/RialtoCommonModule.cpp
        source/WebAudioPlayerClient.cpp
        source/WebAudioPlayerModuleService.cpp
)

set_property (TARGET
        RialtoServerIpc
        PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_include_directories (
        RialtoServerIpc

        PUBLIC
        public

        PRIVATE
        include
        $<TARGET_PROPERTY:RialtoServerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMain,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerService,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcServer,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries (
        RialtoServerIpc

        PRIVATE
        RialtoIpcServer
        RialtoIpcCommon
        RialtoServerMain
        RialtoServerService
        protobuf::libprotobuf
        Threads::Threads
)
