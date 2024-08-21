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

add_gtests (
        RialtoClientIpcUnitTests

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
        mediaPipelineIpc/SetImmediateOutputTest.cpp
        mediaPipelineIpc/GetImmediateOutputTest.cpp
        mediaPipelineIpc/RenderFrameTest.cpp
        mediaPipelineIpc/GetVolumeTest.cpp
        mediaPipelineIpc/SetVolumeTest.cpp
        mediaPipelineIpc/GetMuteTest.cpp
        mediaPipelineIpc/SetMuteTest.cpp
        mediaPipelineIpc/FlushTest.cpp
        mediaPipelineIpc/SetSourcePositionTest.cpp
        mediaPipelineIpc/ProcessAudioGapTest.cpp

        # MediaPipelineCapabilitiesIpc tests
        mediaPipelineCapabilitiesIpc/MediaPipelineCapabilitiesIpcTest.cpp

        # ControlIpc tests
        controlIpc/base/ControlIpcTestBase.cpp
        controlIpc/CreateTest.cpp
        controlIpc/GetSharedMemoryTest.cpp
        controlIpc/NotifyApplicationStateTest.cpp
        controlIpc/PingTest.cpp
        controlIpc/RegisterClientTest.cpp

        # IpcClient tests
        ipcClient/base/IpcClientTestBase.cpp
        ipcClient/IpcClientTest.cpp

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
        mediaKeysIpc/ContainsKeyTest.cpp
        mediaKeysIpc/SetDrmHeaderTest.cpp
        mediaKeysIpc/DeleteDrmStoreTest.cpp
        mediaKeysIpc/DeleteKeyStoreTest.cpp
        mediaKeysIpc/GetDrmStoreHashTest.cpp
        mediaKeysIpc/GetKeyStoreHashTest.cpp
        mediaKeysIpc/GetLdlSessionsLimitTest.cpp
        mediaKeysIpc/GetLastDrmErrorTest.cpp
        mediaKeysIpc/GetDrmTimeTest.cpp
        mediaKeysIpc/ReleaseKeySessionTest.cpp

        # MediaKeysCapabilitiesIpc tests
        mediaKeysCapabilitiesIpc/CreateTest.cpp
        mediaKeysCapabilitiesIpc/KeySystemsTest.cpp
        mediaKeysCapabilitiesIpc/CertificateTest.cpp

        # WebAudioPlayer tests
        webAudioPlayerIpc/base/WebAudioPlayerIpcTestBase.cpp
        webAudioPlayerIpc/CallbackTest.cpp
        webAudioPlayerIpc/CreateTest.cpp
        webAudioPlayerIpc/CreateTest.cpp
        webAudioPlayerIpc/PlayPauseTest.cpp
        webAudioPlayerIpc/SetVolumeTest.cpp
        webAudioPlayerIpc/GetVolumeTest.cpp
        webAudioPlayerIpc/SetEosTest.cpp
        webAudioPlayerIpc/GetBufferDelayTest.cpp
        webAudioPlayerIpc/WriteBufferTest.cpp
        webAudioPlayerIpc/GetDeviceInfoTest.cpp
        webAudioPlayerIpc/GetBufferAvailable.cpp
        )

target_include_directories(
        RialtoClientIpcUnitTests

        PRIVATE
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcClient,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientIpcImpl,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoUnittestCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicClientMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonMatchers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonProtoUtils,INTERFACE_INCLUDE_DIRECTORIES>

        # Test Bases
        ipcModuleBase
        mediaPipelineIpc/base
        controlIpc/base
        mediaKeysIpc/base
        webAudioPlayerIpc/base
        )

target_link_libraries(
        RialtoClientIpcUnitTests

        # # Link application source
        RialtoClientIpcImpl
        RialtoIpcClient
        RialtoProtobuf
        )

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoClientIpcUnitTests

        gcov
        )
endif()
