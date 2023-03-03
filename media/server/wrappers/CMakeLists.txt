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

if( NOT CMAKE_BUILD_FLAG STREQUAL "UnitTests" )

    find_package( ocdm REQUIRED )

    add_library(
        RialtoServerWrappers
        STATIC

        source/Ocdm.cpp
        source/OcdmCommon.cpp
        source/OcdmSystem.cpp
        source/OcdmSession.cpp
        source/RdkGstreamerUtilsWrapper.cpp
        )

    target_include_directories(
        RialtoServerWrappers

        PUBLIC
        public

        PRIVATE
        include
        $<TARGET_PROPERTY:RialtoServerCommon,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:RialtoPlayerPublic,INTERFACE_INCLUDE_DIRECTORIES>
        ${GStreamerApp_INCLUDE_DIRS}

        ${OCdm_INCLUDE_DIRS}
        )


    set_property(
        TARGET RialtoServerWrappers
        PROPERTY POSITION_INDEPENDENT_CODE ON

        )

    target_link_libraries(
        RialtoServerWrappers

        PRIVATE
        RialtoLogging
        ocdm::ocdm
        rdkgstreamerutils
        ${CMAKE_DL_LIBS}

        )

else()

    add_library(
        RialtoServerWrappers
        INTERFACE
    )

    target_include_directories(
        RialtoServerWrappers

        INTERFACE
        public
    )

endif()
