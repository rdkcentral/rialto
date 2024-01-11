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

find_package( jsoncpp REQUIRED )

add_gtests (
        RialtoServerManagerUnitTests

        # gtest code
        unittests/common/HealthcheckServiceTests.cpp
        unittests/common/SessionServerAppManagerTests.cpp
        unittests/common/SessionServerAppManagerTestsFixture.cpp
        unittests/common/SessionServerAppTests.cpp
        unittests/common/SessionServerAppTestsFixture.cpp
        unittests/common/UtilsTests.cpp

        unittests/ipc/IpcTests.cpp
        unittests/ipc/IpcTestsFixture.cpp

        unittests/service/ConfigHelperTests.cpp
        unittests/service/ConfigReaderTests.cpp
        unittests/service/ServerManagerServiceTests.cpp
        unittests/service/ServerManagerServiceTestsFixture.cpp
        )

add_subdirectory(matchers)
add_subdirectory(stubs)

target_include_directories(
        RialtoServerManagerUnitTests

        PRIVATE
        $<TARGET_PROPERTY:RialtoServerManagerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerManagerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerManagerIpc,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerManager,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerManagerMatchers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerManagerStub,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoUnittestCommonMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoWrappers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoWrappersMocks,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoLogging,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcClient,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcServer,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoTestCommonMatchers,INTERFACE_INCLUDE_DIRECTORIES>

        mocks

        ../../../serverManager/common/source
        ../../../serverManager/ipc/source
        ../../../serverManager/service/source

        ${JSONCPP_INCLUDE_DIRS}
)

target_link_libraries(
        RialtoServerManagerUnitTests

        # # Link application source
        RialtoServerManagerMatchers
        RialtoServerManagerStub
        RialtoServerManagerCommon
        RialtoServerManagerIpc
        RialtoServerManager
        Threads::Threads
        )

if ( COVERAGE_ENABLED )
    target_link_libraries(
        RialtoServerManagerUnitTests

        gcov
        )
endif()
