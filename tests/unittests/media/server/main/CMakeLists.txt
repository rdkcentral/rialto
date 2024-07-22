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
        RialtoServerMainUnitTests

        # gtest code
        activeRequests/ActiveRequestsTests.cpp

        control/ControlServerInternalTests.cpp

        dataReader/DataReaderFactoryTests.cpp
        dataReader/DataReaderV1Tests.cpp
        dataReader/DataReaderV2Tests.cpp

        heartbeatProcedure/HeartbeatProcedureTests.cpp

        mediaPipeline/base/MediaPipelineTestBase.cpp
        mediaPipeline/CreateTest.cpp
        mediaPipeline/LoadTest.cpp
        mediaPipeline/SourceTest.cpp
        mediaPipeline/CallbackTest.cpp
        mediaPipeline/MiscellaneousFunctionsTest.cpp
        mediaPipeline/HaveDataTest.cpp
        mediaPipeline/FlushTest.cpp
        mediaPipeline/SetSourcePositionTest.cpp
        mediaPipeline/ProcessAudioGapTest.cpp

        mediaPipelineCapabilities/MediaPipelineCapabilitiesTest.cpp

        mediaKeys/base/MediaKeysTestBase.cpp
        mediaKeys/CreateTest.cpp
        mediaKeys/CreateKeySessionTest.cpp
        mediaKeys/DecryptTest.cpp
        mediaKeys/GenerateRequestTest.cpp
        mediaKeys/LoadSessionTest.cpp
        mediaKeys/UpdateSessionTest.cpp
        mediaKeys/CloseKeySessionTest.cpp
        mediaKeys/RemoveKeySessionTest.cpp
        mediaKeys/GetCdmKeySessionIdTest.cpp
        mediaKeys/ContainsKeyTest.cpp
        mediaKeys/SetDrmHeaderTest.cpp
        mediaKeys/DeleteDrmStoreTest.cpp
        mediaKeys/DeleteKeyStoreTest.cpp
        mediaKeys/GetDrmStoreHashTest.cpp
        mediaKeys/GetKeyStoreHashTest.cpp
        mediaKeys/GetLdlSessionsLimitTest.cpp
        mediaKeys/GetDrmTimeTest.cpp
        mediaKeys/GetLastDrmErrorTest.cpp
        mediaKeys/SelectKeyIdTest.cpp
        mediaKeys/IsNetflixPlayreadyKeySystemTest.cpp
        mediaKeys/PingTest.cpp
        mediaKeys/ReleaseKeySessionTest.cpp

        mediaKeysCapabilities/CreateTest.cpp
        mediaKeysCapabilities/KeySystemsTest.cpp
        mediaKeysCapabilities/CertificateTest.cpp

        mediaKeySession/base/MediaKeySessionTestBase.cpp
        mediaKeySession/CreateTest.cpp
        mediaKeySession/DecryptTest.cpp
        mediaKeySession/DecryptBufferTest.cpp
        mediaKeySession/GenerateRequestTest.cpp
        mediaKeySession/LoadSessionTest.cpp
        mediaKeySession/UpdateSessionTest.cpp
        mediaKeySession/CloseKeySessionTest.cpp
        mediaKeySession/RemoveKeySessionTest.cpp
        mediaKeySession/CallbacksTest.cpp
        mediaKeySession/GetCdmKeySessionIdTest.cpp
        mediaKeySession/ContainsKeyTest.cpp
        mediaKeySession/SetDrmHeaderTest.cpp
        mediaKeySession/GetLastDrmErrorTest.cpp
        mediaKeySession/SelectKeyIdTest.cpp
        mediaKeySession/IsNetflixPlayreadyKeySystemTest.cpp

        sharedMemoryBuffer/SharedMemoryBufferTestsFixture.cpp
        sharedMemoryBuffer/SharedMemoryBufferTests.cpp

        needMediaData/NeedMediaDataTestsFixture.cpp
        needMediaData/NeedMediaDataTests.cpp

        mainThread/MainThreadTest.cpp

        webAudioPlayer/base/WebAudioPlayerTestBase.cpp
        webAudioPlayer/CreateTest.cpp
        webAudioPlayer/MiscellaneousFunctionsTest.cpp
        webAudioPlayer/BufferApiTest.cpp
        webAudioPlayer/CallbackTest.cpp
        )

target_include_directories(
        RialtoServerMainUnitTests

        PRIVATE
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerGstPlayer,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMain,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerCommon,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicClientMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMisc,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ExternalLibraryMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoUnittestCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoWrappersMocks,INTERFACE_INCLUDE_DIRECTORIES>

        # Test Bases
        mediaKeys/base
        mediaKeySession/base
        mediaPipeline/base
        webAudioPlayer/base
        )

target_link_libraries(
        RialtoServerMainUnitTests

        # # Link application source
        RialtoServerMain
        RialtoPlayerCommon
        RialtoCommonMisc
        )

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoServerMainUnitTests

        gcov
        )
endif()
