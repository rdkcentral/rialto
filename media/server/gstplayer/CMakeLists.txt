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

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

find_package( PkgConfig REQUIRED )
pkg_check_modules( GStreamerApp REQUIRED IMPORTED_TARGET gstreamer-app-1.0 gstreamer-pbutils-1.0 gstreamer-audio-1.0)

add_library(
        RialtoServerGstPlayer
        STATIC

        source/tasks/AttachSamples.cpp
        source/tasks/AttachSource.cpp
        source/tasks/EnoughData.cpp
        source/tasks/Eos.cpp
        source/tasks/FinishSetupSource.cpp
        source/tasks/HandleBusMessage.cpp
        source/tasks/NeedData.cpp
        source/tasks/Pause.cpp
        source/tasks/Play.cpp
        source/tasks/PlayerTaskFactory.cpp
        source/tasks/ReadShmDataAndAttachSamples.cpp
        source/tasks/RenderFrame.cpp
        source/tasks/ReportPosition.cpp
        source/tasks/CheckAudioUnderflow.cpp
        source/tasks/SetPlaybackRate.cpp
        source/tasks/SetPosition.cpp
        source/tasks/SetupElement.cpp
        source/tasks/SetupSource.cpp
        source/tasks/SetVideoGeometry.cpp
        source/tasks/SetVolume.cpp
        source/tasks/Shutdown.cpp
        source/tasks/Stop.cpp
        source/tasks/Underflow.cpp

        source/GstDispatcherThread.cpp
        source/GstPlayer.cpp
        source/GstProtectionMetadata.cpp
        source/GstProtectionMetadataWrapper.cpp
        source/GstWrapper.cpp
        source/GlibWrapper.cpp
        source/GstSrc.cpp
        source/Utils.cpp
        source/WorkerThread.cpp
        source/GstCapabilities.cpp
        source/GstDecryptor.cpp
        source/GstLogForwarding.cpp
        )

target_include_directories(
        RialtoServerGstPlayer

        PUBLIC
        public

        PRIVATE
        include
        include/tasks
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoServerMain,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        ${GStreamerApp_INCLUDE_DIRS}
        )

if(PERFETTO_TRACING)
    MESSAGE("Perfetto Tracing") 
target_include_directories(
        RialtoServerGstPlayer

        PRIVATE
        $<TARGET_PROPERTY:RialtoPerfetto,INTERFACE_INCLUDE_DIRECTORIES>
        )
endif()

set_property(
        TARGET RialtoServerGstPlayer
        PROPERTY POSITION_INDEPENDENT_CODE ON

        )

target_link_libraries(
        RialtoServerGstPlayer

        PRIVATE
        RialtoLogging
        RialtoServerMain
        RialtoCommon
        ${GStreamerApp_LIBRARIES}
        )

if(PERFETTO_TRACING)
target_link_libraries(
        RialtoServerGstPlayer

        PRIVATE 
        RialtoPerfetto
        )

target_compile_definitions(
        RialtoServerGstPlayer

        PRIVATE
        PERFETTO_TRACING=1
    )
endif()
