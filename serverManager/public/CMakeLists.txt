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

set( CMAKE_CXX_STANDARD 14 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )

add_library(
        RialtoServerManagerPublic
        INTERFACE
    )

target_include_directories(
        RialtoServerManagerPublic

        INTERFACE
        include
    )

if( NOT CMAKE_BUILD_TYPE STREQUAL "UnitTests" )
        include( GNUInstallDirs )
        set (LIB_RIALTO_SERVER_MANAGER_PUBLIC_HEADERS
                include/IServerManagerService.h
                include/IStateObserver.h
                include/LoggingLevels.h
                include/ServerManagerServiceFactory.h
        )
        install (
                FILES ${LIB_RIALTO_SERVER_MANAGER_PUBLIC_HEADERS}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rialto/servermanager
        )
endif()
