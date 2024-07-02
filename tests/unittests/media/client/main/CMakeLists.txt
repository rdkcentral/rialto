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
        RialtoClientUnitTests

        # MediaPipeline tests
        mediaPipeline/base/MediaPipelineTestBase.cpp
        mediaPipeline/CreateTest.cpp
        mediaPipeline/SourceTest.cpp
        mediaPipeline/LoadTest.cpp
        mediaPipeline/CallbackTest.cpp
        mediaPipeline/SetVideoWindowTest.cpp
        mediaPipeline/DataTest.cpp
        mediaPipeline/PlayPauseTest.cpp
        mediaPipeline/SetPositionTest.cpp
        mediaPipeline/SetPlaybackRateTest.cpp
        mediaPipeline/GetPositionTest.cpp
        mediaPipeline/RenderFrameTest.cpp
        mediaPipeline/SetVolumeTest.cpp
        mediaPipeline/GetVolumeTest.cpp
        mediaPipeline/SetMuteTest.cpp
        mediaPipeline/GetMuteTest.cpp
        mediaPipeline/FlushTest.cpp
        mediaPipeline/SetSourcePositionTest.cpp
        mediaPipeline/MediaPipelineProxyTest.cpp

        # MediaPipelineCapabilities tests
        mediaPipelineCapabilities/MediaPipelineCapabilitiesTest.cpp

        # Control tests
        control/ControlTest.cpp
        control/ControlRegisterLogHandlerTest.cpp

        # MediaKeys tests
        mediaKeys/CreateTest.cpp
        mediaKeys/KeySessionTest.cpp
        mediaKeys/NetflixKeySessionTest.cpp

        # MediaKeysCapabilities tests
        mediaKeysCapabilities/CreateTest.cpp
        mediaKeysCapabilities/KeySystemsTest.cpp
        mediaKeysCapabilities/CertificateTest.cpp

        # ClientController tests
        clientController/CreateTest.cpp
        clientController/MemoryManagementTest.cpp

        # WebAudioPlayer tests
        webAudioPlayer/base/WebAudioPlayerTestBase.cpp
        webAudioPlayer/CreateTest.cpp
        webAudioPlayer/CallbackTest.cpp
        webAudioPlayer/PlayPauseTest.cpp
        webAudioPlayer/SetVolumeTest.cpp
        webAudioPlayer/GetVolumeTest.cpp
        webAudioPlayer/SetEosTest.cpp
        webAudioPlayer/GetBufferDelayTest.cpp
        webAudioPlayer/GetDeviceInfoTest.cpp
        webAudioPlayer/GetBufferAvailableTest.cpp
        webAudioPlayer/WriteBufferTest.cpp
        webAudioPlayer/WebAudioPlayerProxyTest.cpp
        )

target_include_directories(
        RialtoClientUnitTests

        PRIVATE
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientIpcImpl,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicClientMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMisc,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonMatchers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>

        mocks

        # Test Bases
        mediaPipeline/base
        webAudioPlayer/base
        )

target_link_libraries(
        RialtoClientUnitTests

        RialtoCommonMisc
        # # Link application source
        RialtoClient
        )

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoClientUnitTests

        gcov
        )
endif()


