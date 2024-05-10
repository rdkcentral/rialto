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
        sessionServer/SessionServerStateChangeTest.cpp
        sessionServer/SessionServerHealthCheckTest.cpp
        sessionServer/SessionServerLogLevelTest.cpp
        mediaPipeline/AudioOnlyPlaybackTest.cpp
        mediaPipeline/AudioSourceSwitchTest.cpp
        mediaPipeline/CreatePipelineTest.cpp
        mediaPipeline/DualVideoPlaybackTest.cpp
        mediaPipeline/EncryptedPlaybackTest.cpp
        mediaPipeline/FailureTests.cpp
        mediaPipeline/HaveDataFailureTest.cpp
        mediaPipeline/MuteTest.cpp
        mediaPipeline/PlaybackTest.cpp
        mediaPipeline/PlayPauseStopFailuresTest.cpp
        mediaPipeline/PositionUpdatesTest.cpp
        mediaPipeline/QosUpdatesTest.cpp
        mediaPipeline/RenderFrameTest.cpp
        mediaPipeline/SetPlaybackRateTest.cpp
        mediaPipeline/SetPositionTest.cpp
        mediaPipeline/SetVideoWindowTest.cpp
        mediaPipeline/SourceTest.cpp
        mediaPipeline/UnderflowTest.cpp
        mediaPipeline/VolumeTest.cpp
        mediaPipeline/WriteSegmentsTest.cpp
        mediaPipeline/RemoveAudioPlaybackTest.cpp
        mediaPipeline/NonFatalPlayerErrorUpdatesTest.cpp
        mediaPipeline/FlushTest.cpp
        mediaPipeline/SetSourcePositionTest.cpp
        mediaKeys/MediaKeysTestMethods.cpp
        mediaKeys/MediaKeysTest.cpp
        mediaKeys/SessionReadyForDecryptionTest.cpp
        mediaKeys/KeyStoreTest.cpp
        mediaKeys/DrmStoreTest.cpp
        mediaKeys/GetDrmInfoTest.cpp
        mediaKeys/KeyApisTest.cpp
        mediaKeys/GetSessionInfoTest.cpp
        mediaKeys/SetDrmHeaderTest.cpp
        mediaKeys/LicenseRenewalTest.cpp
        mediaKeysCapabilities/MediaKeysCapabilitiesTest.cpp
        control/ApplicationStateChangeTest.cpp
        control/HealthcheckTest.cpp
        mediaPipelineCapabilities/MediaPipelineCapabilitiesTest.cpp
        webAudio/WebAudioTestMethods.cpp
        webAudio/WebAudioTest.cpp
        webAudio/WebAudioFailuresTest.cpp
        )

target_include_directories(
        RialtoServerComponentTests

        PRIVATE
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
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
        RialtoProtobuf
        RialtoServerComponentTestsFixtures
        RialtoTestCommonUtils
)
