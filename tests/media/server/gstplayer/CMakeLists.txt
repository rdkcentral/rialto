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

    #GstGenericPlayer unittests
    genericPlayer/tasksTests/AttachSamplesTest.cpp
    genericPlayer/tasksTests/AttachSourceTest.cpp
    genericPlayer/tasksTests/EnoughDataTest.cpp
    genericPlayer/tasksTests/EosTest.cpp
    genericPlayer/tasksTests/FinishSetupSourceTest.cpp
    genericPlayer/tasksTests/HandleBusMessageTest.cpp
    genericPlayer/tasksTests/NeedDataTest.cpp
    genericPlayer/tasksTests/PauseTest.cpp
    genericPlayer/tasksTests/GenericPlayerTaskFactoryTest.cpp
    genericPlayer/tasksTests/PlayTest.cpp
    genericPlayer/tasksTests/ReadShmDataAndAttachSamplesTest.cpp
    genericPlayer/tasksTests/RenderFrameTest.cpp
    genericPlayer/tasksTests/ReportPositionTest.cpp
    genericPlayer/tasksTests/CheckAudioUnderflowTest.cpp
    genericPlayer/tasksTests/SetPlaybackRateTest.cpp
    genericPlayer/tasksTests/SetPositionTest.cpp
    genericPlayer/tasksTests/SetupElementTest.cpp
    genericPlayer/tasksTests/SetupSourceTest.cpp
    genericPlayer/tasksTests/SetVideoGeometryTest.cpp
    genericPlayer/tasksTests/SetVolumeTest.cpp
    genericPlayer/tasksTests/ShutdownTest.cpp
    genericPlayer/tasksTests/StopTest.cpp
    genericPlayer/tasksTests/UnderflowTest.cpp

    genericPlayer/common/Matchers.cpp
    genericPlayer/common/GstGenericPlayerTestCommon.cpp

    genericPlayer/CreateTest.cpp
    genericPlayer/GstGenericPlayerPrivateTest.cpp
    genericPlayer/GstGenericPlayerTest.cpp
    genericPlayer/InitTest.cpp
    genericPlayer/WorkerThreadTest.cpp
    genericPlayer/GstCapabilitiesTest.cpp

    #GstSrc unittests
    rialtoSrc/CreateTest.cpp
    rialtoSrc/InitTest.cpp
    rialtoSrc/AppSrcTest.cpp

    #Decyptor unittests
    decryptor/CreateTest.cpp
    decryptor/DecryptTest.cpp

    #ProtectionMetadata unittests
    protectionMetadata/ProtectionMetadataTest.cpp

    #ProtectionMetadata unittests
    dispatcherThread/GstDispatcherThreadTest.cpp
)

target_include_directories(RialtoServerGstPlayerUnitTests
    PRIVATE
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerGstPlayer,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMisc,INTERFACE_INCLUDE_DIRECTORIES>

        genericPlayer/common/

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
