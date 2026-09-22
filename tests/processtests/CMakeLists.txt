#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2026 Sky UK
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

add_executable(
        RialtoProcessClient
        RialtoProcessClient.cpp
)

target_include_directories(
        RialtoProcessClient
        PRIVATE
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublicClientMocks,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
        RialtoProcessClient
        RialtoClient
        RialtoClientIpcImpl
        RialtoClientCommon
        RialtoPlayerCommon
        RialtoPlayerPublicClientMocks
        GoogleTest::gmock
        GoogleTest::gtest
)

add_gtests(
        RialtoProcessTests
        PerInstanceSharedMemoryProcessTests.cpp
)

add_dependencies(RialtoProcessTests RialtoProcessClient RialtoServer)

target_compile_definitions(
        RialtoProcessTests
        PRIVATE
        RIALTO_PROCESS_CLIENT_PATH="$<TARGET_FILE:RialtoProcessClient>"
        RIALTO_SERVER_PATH="$<TARGET_FILE:RialtoServer>"
)

target_include_directories(
        RialtoProcessTests
        PRIVATE
        $<TARGET_PROPERTY:RialtoServerManagerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
        RialtoProcessTests
        RialtoServerManager
        RialtoCommon
        RialtoLogging
        Threads::Threads
)
