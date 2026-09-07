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
add_library (
        RialtoServerManagerCommon STATIC
        source/HealthcheckService.cpp
        source/HealthcheckServiceFactory.cpp
        source/SessionServerAppFactory.cpp
        source/SessionServerApp.cpp
        source/SessionServerAppManager.cpp
        source/SessionServerAppManagerFactory.cpp
        source/Utils.cpp
)
set_target_properties (
        RialtoServerManagerCommon
        PROPERTIES LINK_FLAGS "-Wl,--unresolved-symbols=report-all" POSITION_INDEPENDENT_CODE ON
)
target_include_directories (
        RialtoServerManagerCommon
        PUBLIC
        include
        PRIVATE
        $<TARGET_PROPERTY:RialtoServerManagerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoIpcCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoWrappers,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries (
        RialtoServerManagerCommon
        PRIVATE
        RialtoLogging
        RialtoCommon
        RialtoIpcCommon
        RialtoWrappers
        RialtoServerManagerIpc
        Threads::Threads
)
