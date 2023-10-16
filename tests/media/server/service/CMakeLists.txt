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
        RialtoServerServiceUnitTests

        # gtest code
        playbackService/PlaybackServiceTestsFixture.cpp
        playbackService/PlaybackServiceTests.cpp

        mediaPipelineService/MediaPipelineServiceTestsFixture.cpp
        mediaPipelineService/MediaPipelineServiceTests.cpp

        cdmService/CdmServiceTestsFixture.cpp
        cdmService/CdmServiceTests.cpp

        sessionServerManager/SessionServerManagerTestsFixture.cpp
        sessionServerManager/SessionServerManagerTests.cpp

        webAudioPlayerService/WebAudioPlayerServiceTestsFixture.cpp
        webAudioPlayerService/WebAudioPlayerServiceTests.cpp

        controlService/ControlServiceTestsFixture.cpp
        controlService/ControlServiceTests.cpp
        )

target_include_directories(
        RialtoServerServiceUnitTests

        PRIVATE
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerIpc,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMain,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerService,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerGstPlayer,INTERFACE_INCLUDE_DIRECTORIES>
        ../../../../media/server/service/source/
        )

target_link_libraries(
        RialtoServerServiceUnitTests

        # # Link application source
        RialtoServerService
        )

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoServerServiceUnitTests

        gcov
        )
endif()
