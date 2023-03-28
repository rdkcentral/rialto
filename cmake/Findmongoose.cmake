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

if( NOT NATIVE_BUILD )
  find_path( MONGOOSE_INCLUDE_DIR NAMES mongoose.h )
  find_library( MONGOOSE_LIBRARY NAMES libmongoose.so )

#  message( "MONGOOSE_INCLUDE_DIR include dir = ${MONGOOSE_INCLUDE_DIR}" )
#  message( "MONGOOSE_LIBRARY lib = ${MONGOOSE_LIBRARY}" )

  include( FindPackageHandleStandardArgs )

  find_package_handle_standard_args( MONGOOSE DEFAULT_MSG MONGOOSE_LIBRARY MONGOOSE_INCLUDE_DIR )

  mark_as_advanced( MONGOOSE_INCLUDE_DIR MONGOOSE_LIBRARY )

  if( MONGOOSE_FOUND )
       set( MONGOOSE_LIBRARIES ${MONGOOSE_LIBRARY} )
       set( MONGOOSE_INCLUDE_DIRS ${MONGOOSE_INCLUDE_DIR} )
  endif()

  if( MONGOOSE_FOUND AND NOT TARGET mongoose )
       add_library( mongoose SHARED IMPORTED )
       set_target_properties( mongoose PROPERTIES
               IMPORTED_LOCATION "${MONGOOSE_LIBRARY}"
               INTERFACE_INCLUDE_DIRECTORIES "${MONGOOSE_INCLUDE_DIR}" )
  endif()
else()
  include(ExternalProject)
  set_property(DIRECTORY PROPERTY EP_BASE "${CMAKE_CURRENT_BINARY_DIR}/third-party")
  ExternalProject_Add(
    mongoose-source
    URL https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/mongoose/mongoose-2.6.tgz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND  COPT="-Wl,--no-as-needed" make linux
    INSTALL_COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/mongoose-source/_mongoose.so
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/mongoose-source/libmongoose.so
  )

  ExternalProject_Get_Property( mongoose-source SOURCE_DIR )

  add_library(mongoose SHARED IMPORTED)
  set_target_properties(mongoose
  PROPERTIES
    IMPORTED_LOCATION  "${SOURCE_DIR}/libmongoose.so"
    INTERFACE_INCLUDE_DIRECTORIES "${SOURCE_DIR}"
    IMPORTED_NO_SONAME TRUE
  )

  add_dependencies( mongoose mongoose-source )

  unset( SOURCE_DIR )
endif()
