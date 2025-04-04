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
        RialtoServerComponentTests

        # Main
        main.cpp

        # gtest code
        control/ApplicationStateChangeTest.cpp
        control/HealthcheckTest.cpp
        mediaKeys/DrmStoreTest.cpp
        mediaKeys/GetDrmInfoTest.cpp
        mediaKeys/GetSessionInfoTest.cpp
        mediaKeys/KeyApisTest.cpp
        mediaKeys/KeyStoreTest.cpp
        mediaKeys/LicenseRenewalTest.cpp
        mediaKeys/MediaKeysTest.cpp
        mediaKeys/MediaKeysTestMethods.cpp
        mediaKeys/SessionReadyForDecryptionTest.cpp
        mediaKeys/SetDrmHeaderTest.cpp
        mediaKeys/GetMetricSystemDataTest.cpp
        mediaKeysCapabilities/MediaKeysCapabilitiesTest.cpp
        mediaPipeline/AudioOnlyPlaybackTest.cpp
        mediaPipeline/AudioSourceSwitchTest.cpp
        mediaPipeline/CreatePipelineTest.cpp
        mediaPipeline/DualVideoPlaybackTest.cpp
        mediaPipeline/EncryptedPlaybackTest.cpp
        mediaPipeline/FailureTests.cpp
        mediaPipeline/FlushTest.cpp
        mediaPipeline/HaveDataFailureTest.cpp
        mediaPipeline/MuteTest.cpp
        mediaPipeline/NonFatalPlayerErrorUpdatesTest.cpp
        mediaPipeline/PipelinePropertyTest.cpp
        mediaPipeline/PlayPauseStopFailuresTest.cpp
        mediaPipeline/PlaybackTest.cpp
        mediaPipeline/PositionUpdatesTest.cpp
        mediaPipeline/ProcessAudioGapTest.cpp
        mediaPipeline/QosUpdatesTest.cpp
        mediaPipeline/RemoveAudioPlaybackTest.cpp
        mediaPipeline/RenderFrameTest.cpp
        mediaPipeline/SetPlaybackRateTest.cpp
        mediaPipeline/SetPositionTest.cpp
        mediaPipeline/SetSourcePositionTest.cpp
        mediaPipeline/SetVideoWindowTest.cpp
        mediaPipeline/SourceTest.cpp
        mediaPipeline/UnderflowTest.cpp
        mediaPipeline/VolumeTest.cpp
        mediaPipeline/WriteSegmentsTest.cpp
        mediaPipelineCapabilities/MediaPipelineCapabilitiesTest.cpp
        recovery/RecoveryTest.cpp
        sessionServer/SessionServerHealthCheckTest.cpp
        sessionServer/SessionServerLogLevelTest.cpp
        sessionServer/SessionServerStateChangeTest.cpp
        webAudio/WebAudioFailuresTest.cpp
        webAudio/WebAudioTest.cpp
        webAudio/WebAudioTestMethods.cpp
        )

target_include_directories(
        RialtoServerComponentTests

        PRIVATE
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerComponentTestsFixtures,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcServerStub,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcServer,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMain,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonProtoUtils,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonUtils,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
        RialtoServerComponentTests
        RialtoCommon
        RialtoIpcCommon
        RialtoProtobuf
        RialtoServerComponentTestsFixtures
        RialtoTestCommonUtils
)

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoServerComponentTests

        gcov
        )
endif()
