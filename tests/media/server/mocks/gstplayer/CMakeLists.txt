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

set(CMAKE_CXX_STANDARD 14)

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

add_library(
        RialtoServerGstPlayerMocks STATIC

        GenericPlayerTaskFactoryMock.cpp
        GlibWrapperFactoryMock.cpp
        GlibWrapperMock.cpp
        GstDecryptorElementFactoryMock.cpp
        GstDispatcherThreadClientMock.cpp
        GstDispatcherThreadFactoryMock.cpp
        GstDispatcherThreadMock.cpp
        GstGenericPlayerPrivateMock.cpp
        GstProtectionMetadataWrapperFactoryMock.cpp
        GstProtectionMetadataWrapperMock.cpp
        GstSrcFactoryMock.cpp
        GstSrcMock.cpp
        GstWebAudioPlayerPrivateMock.cpp
        GstWrapperFactoryMock.cpp
        GstWrapperMock.cpp
        PlayerTaskMock.cpp
        WebAudioPlayerTaskFactoryMock.cpp
        WorkerThreadFactoryMock.cpp
        WorkerThreadMock.cpp
        )

target_include_directories(
        RialtoServerGstPlayerMocks

        PUBLIC
        ./

        PRIVATE
        $<TARGET_PROPERTY:RialtoServerGstPlayer,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMain,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerWrappers,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        )

set_property(
        TARGET RialtoServerGstPlayerMocks
        PROPERTY POSITION_INDEPENDENT_CODE ON

        )

target_link_libraries(
        RialtoServerGstPlayerMocks

        PRIVATE
        GoogleTest::gmock

        )
