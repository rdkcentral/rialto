#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 Sky UK
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


##
#
#
#
# TODO: enable only when built for native
#
#
#
#
#
include(ExternalProject)
set_property(DIRECTORY PROPERTY EP_BASE "${CMAKE_CURRENT_BINARY_DIR}/third-party")
ExternalProject_Add(
    mongoose
    URL https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/mongoose/mongoose-2.6.tgz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND  COPT="-Wl,--no-as-needed" make linux
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property( mongoose SOURCE_DIR )

add_library(mongoose-third-party SHARED IMPORTED)
set_target_properties(mongoose-third-party
PROPERTIES
  IMPORTED_LOCATION  "${SOURCE_DIR}/_mongoose.so" 
  INTERFACE_INCLUDE_DIRECTORIES "${SOURCE_DIR}"
)

add_dependencies( mongoose-third-party mongoose )

install (
    FILES  ${SOURCE_DIR}/_mongoose.so
    TYPE LIB
)

unset( INSTALL_DIR )
