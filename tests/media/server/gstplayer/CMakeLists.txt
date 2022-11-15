#
#If not stated otherwise in this file or this component's LICENSE file the
#following copyright and licenses apply:
#
#Copyright 2022 Sky UK
#
#Licensed under the Apache License, Version 2.0(the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#http: // www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.
#

add_gtests(RialtoServerGstPlayerUnitTests

    #GstPlayer unittests
    player/tasksTests/AttachSamplesTest.cpp
    player/tasksTests/AttachSourceTest.cpp
    player/tasksTests/EnoughDataTest.cpp
    player/tasksTests/EosTest.cpp
    player/tasksTests/FinishSetupSourceTest.cpp
    player/tasksTests/HandleBusMessageTest.cpp
    player/tasksTests/NeedDataTest.cpp
    player/tasksTests/PauseTest.cpp
    player/tasksTests/PlayerTaskFactoryTest.cpp
    player/tasksTests/PlayTest.cpp
    player/tasksTests/ReadShmDataAndAttachSamplesTest.cpp
    player/tasksTests/ReportPositionTest.cpp
    player/tasksTests/CheckAudioUnderflowTest.cpp
    player/tasksTests/SetPlaybackRateTest.cpp
    player/tasksTests/SetPositionTest.cpp
    player/tasksTests/SetupElementTest.cpp
    player/tasksTests/SetupSourceTest.cpp
    player/tasksTests/SetVideoGeometryTest.cpp
    player/tasksTests/ShutdownTest.cpp
    player/tasksTests/StopTest.cpp
    player/tasksTests/UnderflowTest.cpp

    player/common/Matchers.cpp
    player/common/GstPlayerTestCommon.cpp

    player/CreateTest.cpp
    player/GstDispatcherThreadTest.cpp
    player/GstPlayerPrivateTest.cpp
    player/GstPlayerTest.cpp
    player/InitTest.cpp
    player/WorkerThreadTest.cpp
    player/GstCapabilitiesTest.cpp

    #GstSrc unittests
    rialtoSrc/CreateTest.cpp
    rialtoSrc/InitTest.cpp
    rialtoSrc/AppSrcTest.cpp
)

target_include_directories(RialtoServerGstPlayerUnitTests
    PRIVATE
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerGstPlayer,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMisc,INTERFACE_INCLUDE_DIRECTORIES>
        
        player/common/

)

target_link_libraries(RialtoServerGstPlayerUnitTests
    # #Link application source
    RialtoServerGstPlayer
    RialtoLogging
    RialtoCommonMisc
)

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoServerGstPlayerUnitTests

        gcov
        )
endif()

set_target_properties(RialtoServerGstPlayerUnitTests PROPERTIES COMPILE_FLAGS "-Wno-write-strings")
