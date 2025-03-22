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

unset(GTEST_MAIN_LIBRARY) # do not link g_test_main

add_gtests (
        RialtoClientComponentTests

        # Main
        main.cpp

        # Component test base
        tests/base/ClientComponentTest.cpp
        tests/base/ControlTestMethods.cpp
        tests/base/ClientLogControlTestMethods.cpp
        tests/base/MediaPipelineTestMethods.cpp
        tests/base/MediaKeysTestMethods.cpp
        tests/base/WebAudioPlayerTestMethods.cpp

        # Control Component tests
        tests/control/ApplicationStateChangeTest.cpp
        tests/control/LogHandlingTest.cpp

        # MSE Component tests
        tests/mse/AudioSwitchingTest.cpp
        tests/mse/AudioVideoPlaybackSequenceTest.cpp
        tests/mse/AudioVideoPlaybackWriteSegmentsTest.cpp
        tests/mse/CreateMediaPipelineFailuresTest.cpp
        tests/mse/DualVideoPlaybackTest.cpp
        tests/mse/FlushTest.cpp
        tests/mse/MediaPipelineCapabilitiesTest.cpp
        tests/mse/MuteTest.cpp
        tests/mse/PipelinePropertyTest.cpp
        tests/mse/PlayPauseStopFailuresTest.cpp
        tests/mse/PlaybackErrorNotificationTest.cpp
        tests/mse/PositionUpdatesTest.cpp
        tests/mse/ProcessAudioGapTest.cpp
        tests/mse/QosNotificationTest.cpp
        tests/mse/RemoveAudioPlaybackTest.cpp
        tests/mse/RenderFrameTest.cpp
        tests/mse/SetPlaybackRateTest.cpp
        tests/mse/SetPositionTest.cpp
        tests/mse/SetSourcePositionTest.cpp
        tests/mse/SetVideoWindowTest.cpp
        tests/mse/SingleSourcePlaybackTest.cpp
        tests/mse/UnderflowNotificationTest.cpp
        tests/mse/VolumeTest.cpp
        tests/mse/WriteSegmentEdgeCasesTest.cpp
        
        # Web Audio Component tests
        tests/webaudio/CreateWebAudioPlayerTest.cpp
        tests/webaudio/WebAudioPlayerPlaybackTest.cpp
        tests/webaudio/GetBufferDelayTest.cpp
        tests/webaudio/CreateWebAudioPlayerFailuresTest.cpp
        tests/webaudio/PlayPauseFailuresTest.cpp
        tests/webaudio/WriteBufferFailuresTest.cpp
        tests/webaudio/WebAudioPlayerVolumeTest.cpp

        # EME Component tests
        tests/eme/SessionReadyForDecryptionTest.cpp
        tests/eme/AudioVideoPlaybackEncryptedSegmentsTest.cpp
        tests/eme/LicenseRenewalTest.cpp
        tests/eme/KeyApisTest.cpp
        tests/eme/DrmStoreTest.cpp
        tests/eme/KeyStoreTest.cpp
        tests/eme/SetDrmHeaderTest.cpp
        tests/eme/GetSessionInfoTest.cpp
        tests/eme/GetDrmInfoTest.cpp
        tests/eme/SelectKeyIdTest.cpp
        tests/eme/SessionFailuresTest.cpp
        tests/eme/MediaKeysCapabilitiesTest.cpp
        tests/eme/GetMetricSystemDataTest.cpp
        )

add_subdirectory(mocks)
add_subdirectory(stubs)

target_include_directories(
        RialtoClientComponentTests

        PRIVATE
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientIpcImpl,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoClientCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcClient,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcServer,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcServerStub,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicClientMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonMatchers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonProtoUtils,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMain,INCLUDE_DIRECTORIES>

        mocks
        tests/base
        )

target_link_libraries(
        RialtoClientComponentTests

        # # Stub source
        RialtoIpcServerStub

        # # Link application source
        RialtoClient
        RialtoClientIpcImpl
        RialtoClientCommon
        RialtoPlayerCommon
        RialtoProtobuf
        )

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoClientComponentTests

        gcov
        )
endif()


