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

unset(GTEST_MAIN_LIBRARY) # do not link g_test_main

add_gtests(RialtoServerGstPlayerUnitTests

    # Main
    main.cpp

    #GstGenericPlayer unittests
    genericPlayer/common/GenericTasksTestsBase.cpp
    genericPlayer/tasksTests/AttachSamplesTest.cpp
    genericPlayer/tasksTests/AttachSourceTest.cpp
    genericPlayer/tasksTests/DeepElementAddedTest.cpp
    genericPlayer/tasksTests/EnoughDataTest.cpp
    genericPlayer/tasksTests/EosTest.cpp
    genericPlayer/tasksTests/FinishSetupSourceTest.cpp
    genericPlayer/tasksTests/FlushTest.cpp
    genericPlayer/tasksTests/HandleBusMessageTest.cpp
    genericPlayer/tasksTests/NeedDataTest.cpp
    genericPlayer/tasksTests/PauseTest.cpp
    genericPlayer/tasksTests/GenericPlayerTaskFactoryTest.cpp
    genericPlayer/tasksTests/PingTest.cpp
    genericPlayer/tasksTests/PlayTest.cpp
    genericPlayer/tasksTests/ReadShmDataAndAttachSamplesTest.cpp
    genericPlayer/tasksTests/RemoveSourceTest.cpp
    genericPlayer/tasksTests/RenderFrameTest.cpp
    genericPlayer/tasksTests/ReportPositionTest.cpp
    genericPlayer/tasksTests/CheckAudioUnderflowTest.cpp
    genericPlayer/tasksTests/SetPlaybackRateTest.cpp
    genericPlayer/tasksTests/SetPositionTest.cpp
    genericPlayer/tasksTests/SetSourcePositionTest.cpp
    genericPlayer/tasksTests/SetupElementTest.cpp
    genericPlayer/tasksTests/SetupSourceTest.cpp
    genericPlayer/tasksTests/SetVideoGeometryTest.cpp
    genericPlayer/tasksTests/SetVolumeTest.cpp
    genericPlayer/tasksTests/SetMuteTest.cpp
    genericPlayer/tasksTests/ShutdownTest.cpp
    genericPlayer/tasksTests/StopTest.cpp
    genericPlayer/tasksTests/UnderflowTest.cpp
    genericPlayer/tasksTests/UpdatePlaybackGroupTest.cpp
    genericPlayer/common/MatchersGenericPlayer.cpp
    genericPlayer/common/GstGenericPlayerTestCommon.cpp

    genericPlayer/CreateTest.cpp
    genericPlayer/GstGenericPlayerPrivateTest.cpp
    genericPlayer/GstGenericPlayerTest.cpp
    genericPlayer/GstDispatcherThreadClientTest.cpp
    genericPlayer/GstCapabilitiesTest.cpp

    #GstWebAudioPlayer unittests
    webAudioPlayer/common/WebAudioTasksTestsBase.cpp
    webAudioPlayer/taskTests/WebAudioPlayerTaskFactoryTest.cpp
    webAudioPlayer/taskTests/PlayTest.cpp
    webAudioPlayer/taskTests/PauseTest.cpp
    webAudioPlayer/taskTests/EosTest.cpp
    webAudioPlayer/taskTests/HandleBusMessageTest.cpp
    webAudioPlayer/taskTests/SetVolumeTest.cpp
    webAudioPlayer/taskTests/ShutdownTest.cpp
    webAudioPlayer/taskTests/StopTest.cpp
    webAudioPlayer/taskTests/SetCapsTest.cpp
    webAudioPlayer/taskTests/WriteBufferTest.cpp

    webAudioPlayer/CreateTest.cpp
    webAudioPlayer/GstWebAudioPlayerPrivateTest.cpp
    webAudioPlayer/GstDispatcherThreadClientTest.cpp
    webAudioPlayer/GstWebAudioPlayerTest.cpp

    webAudioPlayer/common/GstWebAudioPlayerTestCommon.cpp

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

    #GstInit unittests
    gstInit/InitTest.cpp

    #WorkerThread unittests
    workerThread/WorkerThreadTest.cpp

)

target_include_directories(RialtoServerGstPlayerUnitTests
    PRIVATE
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerGstPlayer,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ExternalLibraryMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoUnittestCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMisc,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoWrappersMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonMatchers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonUtils,INTERFACE_INCLUDE_DIRECTORIES>

        genericPlayer/common/
        webAudioPlayer/common/

)

target_link_libraries(RialtoServerGstPlayerUnitTests
    # #Link application source
    RialtoServerGstPlayer
    RialtoLogging
    RialtoCommonMisc
    RialtoTestCommonUtils
)

set_target_properties(RialtoServerGstPlayerUnitTests PROPERTIES COMPILE_FLAGS "-Wno-write-strings")
