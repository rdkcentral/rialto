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

set( Protobuf_IMPORT_DIRS "${CMAKE_SYSROOT}/usr/include" "${CMAKE_CURRENT_LIST_DIR}/../../../../ipc/common/proto/" )
protobuf_generate_cpp( PROTO_SRCS PROTO_HEADERS ../../../../media/client/ipc/proto/mediapipelinemodule.proto ../../../../media/client/ipc/proto/rialtocontrolmodule.proto
        ../../../../media/client/ipc/proto/mediakeysmodule.proto ../../../../media/client/ipc/proto/mediakeyscapabilitiesmodule.proto)

list( GET PROTO_HEADERS 0 MEDIA_PIPELINE_PROTO_HEADER )
list( GET PROTO_HEADERS 1 RIALTO_CONTROL_PROTO_HEADER )
get_filename_component( MEDIA_PIPELINE_PROTO_DIR ${MEDIA_PIPELINE_PROTO_HEADER} DIRECTORY )
get_filename_component( RIALTO_CONTROL_PROTO_DIR ${RIALTO_CONTROL_PROTO_HEADER} DIRECTORY )

add_gtests (
        RialtoClientIpcUnitTests

        # proto files
        ${PROTO_SRCS}
        ${PROTO_HEADERS}

        # IpcModuleBase
        ipcModuleBase/IpcModuleBase.cpp

        # MediaPipelineIpc tests
        mediaPipelineIpc/base/MediaPipelineIpcTestBase.cpp
        mediaPipelineIpc/CreateTest.cpp
        mediaPipelineIpc/LoadTest.cpp
        mediaPipelineIpc/SourceTest.cpp
        mediaPipelineIpc/CallbackTest.cpp
        mediaPipelineIpc/SetVideoWindowTest.cpp
        mediaPipelineIpc/DataTest.cpp
        mediaPipelineIpc/PlayPauseTest.cpp
        mediaPipelineIpc/SetPositionTest.cpp
        mediaPipelineIpc/SetPlaybackRateTest.cpp
        mediaPipelineIpc/GetPositionTest.cpp

        # RialtoControlIpc tests
        rialtoControlIpc/base/RialtoControlIpcTestBase.cpp
        rialtoControlIpc/CreateTest.cpp
        rialtoControlIpc/ConnectionTest.cpp
        rialtoControlIpc/GetSharedMemoryTest.cpp

        # MediaKeysIpc tests
        mediaKeysIpc/base/MediaKeysIpcTestBase.cpp
        mediaKeysIpc/CreateTest.cpp
        mediaKeysIpc/CreateKeySessionTest.cpp
        mediaKeysIpc/GenerateRequestTest.cpp
        mediaKeysIpc/LoadSessionTest.cpp
        mediaKeysIpc/UpdateSessionTest.cpp
        mediaKeysIpc/CloseKeySessionTest.cpp
        mediaKeysIpc/RemoveKeySessionTest.cpp
        mediaKeysIpc/CallbackTest.cpp
        mediaKeysIpc/GetCdmKeySessionIdTest.cpp

        # MediaKeysCapabilitiesIpc tests
        mediaKeysCapabilitiesIpc/CreateTest.cpp
        mediaKeysCapabilitiesIpc/KeySystemsTest.cpp
        )

target_include_directories(
        RialtoClientIpcUnitTests

        PRIVATE
        ${MEDIA_PIPELINE_PROTO_DIR}
        ${RIALTO_CONTROL_PROTO_DIR}
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcClient,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientIpcImpl,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicMocks,INTERFACE_INCLUDE_DIRECTORIES>

        # Test Bases
        ipcModuleBase
        mediaPipelineIpc/base
        rialtoControlIpc/base
        mediaKeysIpc/base
        )

target_link_libraries(
        RialtoClientIpcUnitTests

        # # Link application source
        RialtoClientIpcImpl
        RialtoIpcClient
        )

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoClientIpcUnitTests

        gcov
        )
endif()
