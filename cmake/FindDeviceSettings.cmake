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

find_path( DEVICE_SETTINGS_INCLUDE_DIR NAMES rdk/ds/host.hpp )
find_library( DEVICE_SETTINGS_LIBRARY NAMES libds.so libdshalcli.so libdshalsrv.so )

message( "DEVICE_SETTINGS_INCLUDE_DIR include dir = ${DEVICE_SETTINGS_INCLUDE_DIR}" )
message( "DEVICE_SETTINGS_LIBRARY lib = ${DEVICE_SETTINGS_LIBRARY}" )

include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( DEVICE_SETTINGS DEFAULT_MSG DEVICE_SETTINGS_LIBRARY DEVICE_SETTINGS_INCLUDE_DIR )

mark_as_advanced( DEVICE_SETTINGS_INCLUDE_DIR DEVICE_SETTINGS_LIBRARY )

if( DEVICE_SETTINGS_FOUND )
    set( DEVICE_SETTINGS_LIBRARIES ${DEVICE_SETTINGS_LIBRARY} )
    set( DEVICE_SETTINGS_INCLUDE_DIRS
            ${DEVICE_SETTINGS_INCLUDE_DIR}/rdk/ds
            ${DEVICE_SETTINGS_INCLUDE_DIR}/rdk/halif/ds-hal
    )
endif()

if( DEVICE_SETTINGS_FOUND AND NOT TARGET DeviceSettings )
    add_library( DeviceSettings SHARED IMPORTED )
    set_target_properties( DeviceSettings PROPERTIES
            IMPORTED_LOCATION "${DEVICE_SETTINGS_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${DEVICE_SETTINGS_INCLUDE_DIR}/rdk/ds ${DEVICE_SETTINGS_INCLUDE_DIR}/rdk/halif/ds-hal" )
endif()
